#include "pch.h"
#include "TensorRTInferenceEngine.h"
#include "DeviceResources.h"
#include "StrUtils.h"
#include "Win32Utils.h"
#include <cuda_d3d11_interop.h>

#pragma warning(push)
// C4996: 'nvinfer1::IPluginV2' : 被声明为已否决
#pragma warning(disable: 4996)
#include <NvInfer.h>
#pragma warning(pop)

#pragma comment(lib, "nvinfer.lib")
#pragma comment(lib, "cudart.lib")

namespace Magpie::Core {

struct Logger : public nvinfer1::ILogger {
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

static uint32_t GetMemorySize(const nvinfer1::Dims& dims, uint32_t elemSize) noexcept {
	uint32_t result = elemSize;
	for (int i = 0; i < dims.nbDims; ++i) {
		result *= dims.d[i];
	}
	return result;
}

bool TensorRTInferenceEngine::Initialize(
	const wchar_t* modelPath,
	DeviceResources& deviceResources,
	ID3D11Texture2D* input,
	ID3D11Texture2D** output
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
	{
		D3D11_BUFFER_DESC desc{
			.ByteWidth = pixelCount * 3 * 2,
			.BindFlags = D3D11_BIND_UNORDERED_ACCESS
		};
		HRESULT hr = deviceResources.GetD3DDevice()->CreateBuffer(&desc, nullptr, inputBuffer.put());
		if (FAILED(hr)) {
			return false;
		}
	}

	winrt::com_ptr<ID3D11UnorderedAccessView> uav;
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC desc{
			.Format = DXGI_FORMAT_R16G16_FLOAT,
			.ViewDimension = D3D11_UAV_DIMENSION_BUFFER,
			.Buffer{
				.NumElements = pixelCount / 2 * 3
			}
		};
		HRESULT hr = deviceResources.GetD3DDevice()
			->CreateUnorderedAccessView(inputBuffer.get(), &desc, uav.put());
		if (FAILED(hr)) {
			return false;
		}
	}
	
	cudaGraphicsResource* inputBufferCuda;
	cudaResult = cudaGraphicsD3D11RegisterResource(
		&inputBufferCuda, inputBuffer.get(), cudaGraphicsRegisterFlagsNone);
	if (cudaResult != cudaError_t::cudaSuccess) {
		return false;
	}

	cudaGraphicsResourceSetMapFlags(inputBufferCuda, cudaGraphicsMapFlagsReadOnly);

	cudaResult = cudaGraphicsMapResources(1, &inputBufferCuda, stream);
	if (cudaResult != cudaError_t::cudaSuccess) {
		return false;
	}

	void* inputMem = nullptr;
	size_t numBytes;
	cudaResult = cudaGraphicsResourceGetMappedPointer(&inputMem, &numBytes, inputBufferCuda);
	if (cudaResult != cudaError_t::cudaSuccess) {
		return false;
	}

	cudaGraphicsUnmapResources(1, &inputBufferCuda, stream);

	cudaGraphicsUnregisterResource(inputBufferCuda);

	Logger logger;
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

	void* outputMem = nullptr;
	cudaResult = cudaMalloc(&outputMem, GetMemorySize(outputDims, 2));
	if (cudaResult != cudaError_t::cudaSuccess) {
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

	return false;
}

}
