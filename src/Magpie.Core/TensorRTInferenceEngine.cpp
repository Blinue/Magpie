#include "pch.h"
#include "TensorRTInferenceEngine.h"
#include "DeviceResources.h"
#include "StrUtils.h"
#include "Win32Utils.h"
#include <cuda_d3d11_interop.h>
#include "shaders/TextureToCudaTensorCS.h"
#include "shaders/CudaTensorToTextureCS.h"
#include "BackendDescriptorStore.h"
#include "Logger.h"

#pragma warning(push)
// C4100: “pluginFactory”: 未引用的形参
// C4996: 'nvinfer1::IPluginV2' : 被声明为已否决
#pragma warning(disable: 4100 4996)
#include <NvInfer.h>
#pragma warning(pop)

#pragma comment(lib, "nvinfer.lib")
#pragma comment(lib, "cudart.lib")

namespace Magpie::Core {

struct TensorRTLogger : public nvinfer1::ILogger {
	void log(Severity severity, nvinfer1::AsciiChar const* msg) noexcept override {
		if (severity > Severity::kINFO) {
			return;
		}

		static constexpr const char* severityMap[] = {
			"internal error",
			"error",
			"warning",
			"info",
			"verbose"
		};
		OutputDebugStringA(StrUtils::Concat("[", severityMap[(int)severity], "] ", msg, "\n").c_str());
	}
};

/*static uint32_t GetMemorySize(const nvinfer1::Dims& dims, uint32_t elemSize) noexcept {
	uint32_t result = elemSize;
	for (int i = 0; i < dims.nbDims; ++i) {
		result *= dims.d[i];
	}
	return result;
}*/

template<typename T>
static T GetQueryData(ID3D11DeviceContext* d3dDC, ID3D11Query* query) noexcept {
	T data{};
	while (d3dDC->GetData(query, &data, sizeof(data), 0) != S_OK) {
		Sleep(0);
	}
	return data;
}

bool TensorRTInferenceEngine::Initialize(
	const wchar_t* /*modelPath*/,
	DeviceResources& deviceResources,
	BackendDescriptorStore& descriptorStore,
	ID3D11Texture2D* input,
	ID3D11Texture2D** /*output*/
) {
	int device = 0;
	cudaError_t cudaResult = cudaD3D11GetDevice(&device, deviceResources.GetGraphicsAdapter());
	if (cudaResult != cudaError_t::cudaSuccess) {
		return false;
	}

	cudaResult = cudaSetDevice(device);
	if (cudaResult != cudaError_t::cudaSuccess) {
		return false;
	}

	ID3D11Device5* d3dDevice = deviceResources.GetD3DDevice();
	ID3D11DeviceContext4* d3dDC = deviceResources.GetD3DDC();

	cudaStream_t stream;
	cudaResult = cudaStreamCreate(&stream);
	if (cudaResult != cudaError_t::cudaSuccess) {
		return false;
	}

	SIZE inputSize{};
	{
		D3D11_TEXTURE2D_DESC inputDesc;
		input->GetDesc(&inputDesc);
		inputSize = { (LONG)inputDesc.Width, (LONG)inputDesc.Height };
	}
	uint32_t pixelCount = uint32_t(inputSize.cx * inputSize.cy);
	pixelCount = (pixelCount + 1) / 2 * 2;

	winrt::com_ptr<ID3D11Buffer> inputBuffer;
	winrt::com_ptr<ID3D11Buffer> outputBuffer;
	{
		D3D11_BUFFER_DESC desc{
			.ByteWidth = pixelCount * 3 * 2,
			.BindFlags = D3D11_BIND_UNORDERED_ACCESS
		};
		HRESULT hr = d3dDevice->CreateBuffer(&desc, nullptr, inputBuffer.put());
		if (FAILED(hr)) {
			return false;
		}

		desc.ByteWidth *= 4;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		hr = d3dDevice->CreateBuffer(&desc, nullptr, outputBuffer.put());
		if (FAILED(hr)) {
			return false;
		}
	}

	winrt::com_ptr<ID3D11UnorderedAccessView> inputUav;
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC desc{
			.Format = DXGI_FORMAT_R16_FLOAT,
			.ViewDimension = D3D11_UAV_DIMENSION_BUFFER,
			.Buffer{
				.NumElements = pixelCount * 3
			}
		};
		HRESULT hr = d3dDevice->CreateUnorderedAccessView(inputBuffer.get(), &desc, inputUav.put());
		if (FAILED(hr)) {
			return false;
		}
	}
	winrt::com_ptr<ID3D11ShaderResourceView> outputSrv;
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC desc{
			.Format = DXGI_FORMAT_R16_FLOAT,
			.ViewDimension = D3D11_SRV_DIMENSION_BUFFER,
			.Buffer{
				.ElementWidth = 2
			}
		};

		HRESULT hr = d3dDevice->CreateShaderResourceView(outputBuffer.get(), &desc, outputSrv.put());
		if (FAILED(hr)) {
			return false;
		}
	}


	winrt::com_ptr<ID3D11ComputeShader> texToTensorShader;
	HRESULT hr = d3dDevice->CreateComputeShader(
		TextureToCudaTensorCS, sizeof(TextureToCudaTensorCS), nullptr, texToTensorShader.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateComputeShader 失败", hr);
		return false;
	}

	winrt::com_ptr<ID3D11ComputeShader> tensorToTexShader;
	hr = d3dDevice->CreateComputeShader(
		CudaTensorToTextureCS, sizeof(CudaTensorToTextureCS), nullptr, tensorToTexShader.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateComputeShader 失败", hr);
		return false;
	}

	{
		ID3D11ShaderResourceView* srv = descriptorStore.GetShaderResourceView(input);
		d3dDC->CSSetShaderResources(0, 1, &srv);
	}

	{
		ID3D11SamplerState* sam = deviceResources.GetSampler(
			D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP);
		d3dDC->CSSetSamplers(0, 1, &sam);
	}
	
	{
		ID3D11UnorderedAccessView* uav = inputUav.get();
		d3dDC->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);
	}

	d3dDC->CSSetShader(texToTensorShader.get(), nullptr, 0);

	/*winrt::com_ptr<ID3D11Query> disjointQuery;
	winrt::com_ptr<ID3D11Query> startQuery;
	winrt::com_ptr<ID3D11Query> endQuery;
	{
		D3D11_QUERY_DESC desc{ .Query = D3D11_QUERY_TIMESTAMP_DISJOINT };
		d3dDevice->CreateQuery(&desc, disjointQuery.put());

		desc.Query = D3D11_QUERY_TIMESTAMP;
		d3dDevice->CreateQuery(&desc, startQuery.put());
		d3dDevice->CreateQuery(&desc, endQuery.put());
	}

	d3dDC->Begin(disjointQuery.get());
	d3dDC->End(startQuery.get());*/

	static constexpr std::pair<uint32_t, uint32_t> BLOCK_SIZE{ 16, 16 };
	std::pair<uint32_t, uint32_t> dispatchCount{
		(inputSize.cx + BLOCK_SIZE.first - 1) / BLOCK_SIZE.first,
		(inputSize.cy + BLOCK_SIZE.second - 1) / BLOCK_SIZE.second
	};
	d3dDC->Dispatch(dispatchCount.first, dispatchCount.second, 1);

	

	/*d3dDC->Flush();

	d3dDC->End(endQuery.get());
	d3dDC->End(disjointQuery.get());

	D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointData =
		GetQueryData<D3D11_QUERY_DATA_TIMESTAMP_DISJOINT>(d3dDC, disjointQuery.get());

	const float toMS = 1000.0f / disjointData.Frequency;

	uint64_t startTimestamp = GetQueryData<uint64_t>(d3dDC, startQuery.get());
	uint64_t endTimestamp = GetQueryData<uint64_t>(d3dDC, endQuery.get());
	float timing = (endTimestamp - startTimestamp) * toMS;
	
	OutputDebugString(std::to_wstring(timing).c_str());*/

	cudaGraphicsResource* inputBufferCuda;
	cudaResult = cudaGraphicsD3D11RegisterResource(
		&inputBufferCuda, inputBuffer.get(), cudaGraphicsRegisterFlagsNone);
	if (cudaResult != cudaError_t::cudaSuccess) {
		return false;
	}

	cudaGraphicsResource* outputBufferCuda;
	cudaResult = cudaGraphicsD3D11RegisterResource(
		&outputBufferCuda, outputBuffer.get(), cudaGraphicsRegisterFlagsNone);
	if (cudaResult != cudaError_t::cudaSuccess) {
		return false;
	}

	cudaGraphicsResourceSetMapFlags(inputBufferCuda, cudaGraphicsMapFlagsReadOnly);

	cudaResult = cudaGraphicsMapResources(1, &inputBufferCuda, stream);
	if (cudaResult != cudaError_t::cudaSuccess) {
		return false;
	}

	void* inputMem = nullptr;
	size_t inputNumBytes;
	cudaResult = cudaGraphicsResourceGetMappedPointer(&inputMem, &inputNumBytes, inputBufferCuda);
	if (cudaResult != cudaError_t::cudaSuccess) {
		return false;
	}

	cudaGraphicsResourceSetMapFlags(inputBufferCuda, cudaGraphicsMapFlagsWriteDiscard);

	cudaResult = cudaGraphicsMapResources(1, &outputBufferCuda, stream);
	if (cudaResult != cudaError_t::cudaSuccess) {
		return false;
	}

	void* outputMem = nullptr;
	size_t outputNumBytes;
	cudaResult = cudaGraphicsResourceGetMappedPointer(&outputMem, &outputNumBytes, outputBufferCuda);
	if (cudaResult != cudaError_t::cudaSuccess) {
		return false;
	}

	TensorRTLogger logger;
	std::unique_ptr<nvinfer1::IRuntime> runtime(nvinfer1::createInferRuntime(logger));
	std::unique_ptr<nvinfer1::ICudaEngine> engine([](nvinfer1::IRuntime* runtime, const wchar_t* modelPath) -> nvinfer1::ICudaEngine* {
		std::vector<uint8_t> engineData;
		Win32Utils::ReadFile(modelPath, engineData);
		if (engineData.empty()) {
			return nullptr;
		}

		return runtime->deserializeCudaEngine(engineData.data(), engineData.size());
	}(runtime.get(), L"engine.trt"));
	
	if (!engine) {
		return false;
	}

	std::unique_ptr<nvinfer1::IExecutionContext> context(engine->createExecutionContext());

	const char* inputName = engine->getIOTensorName(0);
	const char* outputName = engine->getIOTensorName(1);

	const nvinfer1::Dims4 inputDims(1, 3, inputSize.cy, inputSize.cx);
	const nvinfer1::Dims4 outputDims(1, 3, inputSize.cy * 2, inputSize.cx * 2);

	if (!context->setInputShape(inputName, inputDims)) {
		return false;
	}

	if (!context->setTensorAddress(inputName, inputMem)) {
		return false;
	}
	if (!context->setTensorAddress(outputName, outputMem)) {
		return false;
	}

	if (!context->enqueueV3(stream)) {
		return false;
	}

	cudaResult = cudaStreamSynchronize(stream);
	if (cudaResult != cudaError_t::cudaSuccess) {
		return false;
	}

	cudaResult = cudaGraphicsUnmapResources(1, &inputBufferCuda, stream);
	if (cudaResult != cudaError_t::cudaSuccess) {
		return false;
	}
	cudaResult = cudaGraphicsUnmapResources(1, &outputBufferCuda, stream);
	if (cudaResult != cudaError_t::cudaSuccess) {
		return false;
	}

	cudaResult = cudaGraphicsUnregisterResource(inputBufferCuda);
	if (cudaResult != cudaError_t::cudaSuccess) {
		return false;
	}
	cudaResult = cudaGraphicsUnregisterResource(outputBufferCuda);
	if (cudaResult != cudaError_t::cudaSuccess) {
		return false;
	}
	


	return false;
}

}
