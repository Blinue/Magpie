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
#include "DirectXHelper.h"

#pragma comment(lib, "cudart.lib")
#pragma comment(lib, "nvinfer.lib")
#pragma comment(lib, "nvinfer_plugin.lib")

namespace Magpie::Core {

void TensorRTLogger::log(Severity severity, nvinfer1::AsciiChar const* msg) noexcept {
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

TensorRTInferenceEngine::~TensorRTInferenceEngine() {
	if (_inputBufferCuda) {
		cudaGraphicsUnregisterResource(_inputBufferCuda);
	}
	if (_outputBufferCuda) {
		cudaGraphicsUnregisterResource(_outputBufferCuda);
	}
}

bool TensorRTInferenceEngine::Initialize(
	const wchar_t* /*modelPath*/,
	DeviceResources& deviceResources,
	BackendDescriptorStore& descriptorStore,
	ID3D11Texture2D* input,
	ID3D11Texture2D** output
) noexcept {
	int deviceId = 0;
	cudaError_t cudaResult = cudaD3D11GetDevice(&deviceId, deviceResources.GetGraphicsAdapter());
	if (cudaResult != cudaError_t::cudaSuccess) {
		return false;
	}

	cudaResult = cudaSetDevice(deviceId);
	if (cudaResult != cudaError_t::cudaSuccess) {
		return false;
	}

	ID3D11Device5* d3dDevice = deviceResources.GetD3DDevice();
	_d3dDC = deviceResources.GetD3DDC();

	SIZE inputSize{};
	{
		D3D11_TEXTURE2D_DESC inputDesc;
		input->GetDesc(&inputDesc);
		inputSize = { (LONG)inputDesc.Width, (LONG)inputDesc.Height };
	}

	// 创建输出纹理
	winrt::com_ptr<ID3D11Texture2D> outputTex = DirectXHelper::CreateTexture2D(
		d3dDevice,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		inputSize.cx * 2,
		inputSize.cy * 2,
		D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS
	);
	*output = outputTex.get();

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

	_inputTexSrv = descriptorStore.GetShaderResourceView(input);
	_pointSampler = deviceResources.GetSampler(
		D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP);

	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC desc{
			.Format = DXGI_FORMAT_R16_FLOAT,
			.ViewDimension = D3D11_UAV_DIMENSION_BUFFER,
			.Buffer{
				.NumElements = pixelCount * 3
			}
		};

		HRESULT hr = d3dDevice->CreateUnorderedAccessView(
			inputBuffer.get(), &desc, _inputBufferUav.put());
		if (FAILED(hr)) {
			return false;
		}
	}
	
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC desc{
			.Format = DXGI_FORMAT_R16_FLOAT,
			.ViewDimension = D3D11_SRV_DIMENSION_BUFFER,
			.Buffer{
				.NumElements = pixelCount * 4 * 3
			}
		};

		HRESULT hr = d3dDevice->CreateShaderResourceView(
			outputBuffer.get(), &desc, _outputBufferSrv.put());
		if (FAILED(hr)) {
			return false;
		}
	}
	
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC desc{
			.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D
		};
		HRESULT hr = d3dDevice->CreateUnorderedAccessView(
			outputTex.get(), &desc, _outputTexUav.put());
		if (FAILED(hr)) {
			return false;
		}
	}
	
	HRESULT hr = d3dDevice->CreateComputeShader(
		TextureToCudaTensorCS, sizeof(TextureToCudaTensorCS), nullptr, _texToTensorShader.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateComputeShader 失败", hr);
		return false;
	}

	hr = d3dDevice->CreateComputeShader(
		CudaTensorToTextureCS, sizeof(CudaTensorToTextureCS), nullptr, _tensorToTexShader.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateComputeShader 失败", hr);
		return false;
	}

	static constexpr std::pair<uint32_t, uint32_t> TEX_TO_TENSOR_BLOCK_SIZE{ 16, 16 };
	static constexpr std::pair<uint32_t, uint32_t> TENSOR_TO_TEX_BLOCK_SIZE{ 8, 8 };
	_texToTensorDispatchCount = {
		(inputSize.cx + TEX_TO_TENSOR_BLOCK_SIZE.first - 1) / TEX_TO_TENSOR_BLOCK_SIZE.first,
		(inputSize.cy + TEX_TO_TENSOR_BLOCK_SIZE.second - 1) / TEX_TO_TENSOR_BLOCK_SIZE.second
	};
	_tensorToTexDispatchCount = {
		(inputSize.cx * 2 + TENSOR_TO_TEX_BLOCK_SIZE.first - 1) / TENSOR_TO_TEX_BLOCK_SIZE.first,
		(inputSize.cy * 2 + TENSOR_TO_TEX_BLOCK_SIZE.second - 1) / TENSOR_TO_TEX_BLOCK_SIZE.second
	};

	cudaResult = cudaGraphicsD3D11RegisterResource(
		&_inputBufferCuda, inputBuffer.get(), cudaGraphicsRegisterFlagsNone);
	if (cudaResult != cudaError_t::cudaSuccess) {
		return false;
	}

	cudaGraphicsResourceSetMapFlags(_inputBufferCuda, cudaGraphicsMapFlagsReadOnly);

	cudaResult = cudaGraphicsD3D11RegisterResource(
		&_outputBufferCuda, outputBuffer.get(), cudaGraphicsRegisterFlagsNone);
	if (cudaResult != cudaError_t::cudaSuccess) {
		return false;
	}

	cudaGraphicsResourceSetMapFlags(_outputBufferCuda, cudaGraphicsMapFlagsWriteDiscard);

	initLibNvInferPlugins(&_logger, "");
	
	_runtime.reset(nvinfer1::createInferRuntime(_logger));
	{
		std::vector<uint8_t> engineData;
		Win32Utils::ReadFile(L"engine.trt", engineData);
		if (engineData.empty()) {
			return false;
		}

		_engine.reset(_runtime->deserializeCudaEngine(engineData.data(), engineData.size()));
	}
	
	if (!_engine) {
		return false;
	}

	_context.reset(_engine->createExecutionContext());

	_inputName = _engine->getIOTensorName(0);
	_outputName = _engine->getIOTensorName(1);

	const nvinfer1::Dims4 inputDims(1, 3, inputSize.cy, inputSize.cx);
	const nvinfer1::Dims4 outputDims(1, 3, inputSize.cy * 2, inputSize.cx * 2);

	if (!_context->setInputShape(_inputName, inputDims)) {
		return false;
	}

	return true;
}

void TensorRTInferenceEngine::Evaluate() noexcept {
	_d3dDC->CSSetShaderResources(0, 1, &_inputTexSrv);
	_d3dDC->CSSetSamplers(0, 1, &_pointSampler);
	{
		ID3D11UnorderedAccessView* uav = _inputBufferUav.get();
		_d3dDC->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);
	}

	_d3dDC->CSSetShader(_texToTensorShader.get(), nullptr, 0);
	_d3dDC->Dispatch(_texToTensorDispatchCount.first, _texToTensorDispatchCount.second, 1);

	_d3dDC->Flush();

	{
		cudaGraphicsResource* buffers[] = { _inputBufferCuda, _outputBufferCuda };
		cudaError_t cudaResult = cudaGraphicsMapResources(2, buffers);
		if (cudaResult != cudaError_t::cudaSuccess) {
			return;
		}
	}

	void* inputMem = nullptr;
	size_t inputNumBytes;
	cudaError_t cudaResult = cudaGraphicsResourceGetMappedPointer(&inputMem, &inputNumBytes, _inputBufferCuda);
	if (cudaResult != cudaError_t::cudaSuccess) {
		return;
	}

	void* outputMem = nullptr;
	size_t outputNumBytes;
	cudaResult = cudaGraphicsResourceGetMappedPointer(&outputMem, &outputNumBytes, _outputBufferCuda);
	if (cudaResult != cudaError_t::cudaSuccess) {
		return;
	}

	if (!_context->setTensorAddress(_inputName, inputMem)) {
		return;
	}
	if (!_context->setTensorAddress(_outputName, outputMem)) {
		return;
	}

	if (!_context->enqueueV3(NULL)) {
		return;
	}

	{
		cudaGraphicsResource* buffers[] = { _inputBufferCuda, _outputBufferCuda };
		cudaResult = cudaGraphicsUnmapResources(2, buffers);
		if (cudaResult != cudaError_t::cudaSuccess) {
			return;
		}
	}

	{
		ID3D11ShaderResourceView* srv = _outputBufferSrv.get();
		_d3dDC->CSSetShaderResources(0, 1, &srv);
	}
	{
		ID3D11UnorderedAccessView* uav = _outputTexUav.get();
		_d3dDC->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);
	}

	_d3dDC->CSSetShader(_tensorToTexShader.get(), nullptr, 0);
	_d3dDC->Dispatch(_tensorToTexDispatchCount.first, _tensorToTexDispatchCount.second, 1);

	{
		ID3D11ShaderResourceView* srv = nullptr;
		_d3dDC->CSSetShaderResources(0, 1, &srv);
	}
	{
		ID3D11UnorderedAccessView* uav = nullptr;
		_d3dDC->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);
	}
}

}
