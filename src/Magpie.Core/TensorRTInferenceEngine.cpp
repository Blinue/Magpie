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
#include <onnxruntime/core/providers/tensorrt/tensorrt_provider_options.h>
#include <onnxruntime/core/session/onnxruntime_session_options_config_keys.h>

#pragma comment(lib, "cudart.lib")
#pragma comment(lib, "onnxruntime.lib")

namespace Magpie::Core {

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

static void ORT_API_CALL OrtLog(
	void* /*param*/,
	OrtLoggingLevel severity,
	const char* /*category*/,
	const char* /*logid*/,
	const char* /*code_location*/,
	const char* message
) {
	const char* SEVERITIES[] = {
		"verbose",
		"info",
		"warning",
		"error",
		"fatal"
	};
	//Logger::Get().Info(StrUtils::Concat("[", SEVERITIES[severity], "] ", message));
	OutputDebugStringA(StrUtils::Concat("[", SEVERITIES[severity], "] ", message, "\n").c_str());
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

	{
		// TensorRT 要求 Compute Capability 至少为 6.0
		// https://docs.nvidia.com/deeplearning/tensorrt/support-matrix/index.html
		int major, minor;
		cudaResult = cudaDeviceGetAttribute(&major, cudaDevAttrComputeCapabilityMajor, deviceId);
		if (cudaResult != cudaError_t::cudaSuccess) {
			Logger::Get().Error("cudaDeviceGetAttribute 失败");
			return false;
		}

		cudaResult = cudaDeviceGetAttribute(&minor, cudaDevAttrComputeCapabilityMajor, deviceId);
		if (cudaResult != cudaError_t::cudaSuccess) {
			Logger::Get().Error("cudaDeviceGetAttribute 失败");
			return false;
		}

		if (major < 6) {
			Logger::Get().Error(fmt::format("当前设备无法使用 TensorRT\n\tCompute Capability: {}.{}", major, minor));
			return false;
		} else {
			Logger::Get().Info(fmt::format("当前设备 Compute Capability: {}.{}", major, minor));
		}
	}

	cudaResult = cudaSetDevice(deviceId);
	if (cudaResult != cudaError_t::cudaSuccess) {
		return false;
	}

	ID3D11Device5* d3dDevice = deviceResources.GetD3DDevice();
	_d3dDC = deviceResources.GetD3DDC();

	{
		D3D11_TEXTURE2D_DESC inputDesc;
		input->GetDesc(&inputDesc);
		_inputSize = { (LONG)inputDesc.Width, (LONG)inputDesc.Height };
	}

	// 创建输出纹理
	winrt::com_ptr<ID3D11Texture2D> outputTex = DirectXHelper::CreateTexture2D(
		d3dDevice,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		_inputSize.cx * 2,
		_inputSize.cy * 2,
		D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS
	);
	*output = outputTex.get();

	uint32_t pixelCount = uint32_t(_inputSize.cx * _inputSize.cy);
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
		(_inputSize.cx + TEX_TO_TENSOR_BLOCK_SIZE.first - 1) / TEX_TO_TENSOR_BLOCK_SIZE.first,
		(_inputSize.cy + TEX_TO_TENSOR_BLOCK_SIZE.second - 1) / TEX_TO_TENSOR_BLOCK_SIZE.second
	};
	_tensorToTexDispatchCount = {
		(_inputSize.cx * 2 + TENSOR_TO_TEX_BLOCK_SIZE.first - 1) / TENSOR_TO_TEX_BLOCK_SIZE.first,
		(_inputSize.cy * 2 + TENSOR_TO_TEX_BLOCK_SIZE.second - 1) / TENSOR_TO_TEX_BLOCK_SIZE.second
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

	try {
		const auto& ortApi = Ort::GetApi();

		_env = Ort::Env(ORT_LOGGING_LEVEL_INFO, "", OrtLog, nullptr);

		Ort::SessionOptions options;
		options.SetIntraOpNumThreads(1);
		options.AddConfigEntry(kOrtSessionOptionsDisableCPUEPFallback, "1");
		
		OrtTensorRTProviderOptionsV2* trtOptions;
		ortApi.CreateTensorRTProviderOptions(&trtOptions);
		trtOptions->device_id = deviceId;
		trtOptions->trt_fp16_enable = 1;
		trtOptions->trt_engine_cache_enable = 1;
		trtOptions->trt_builder_optimization_level = 5;
		trtOptions->trt_profile_min_shapes = new char[] {"input:1x3x1x1"};
		trtOptions->trt_profile_max_shapes = new char[] {"input:1x3x1080x1920"};
		trtOptions->trt_profile_opt_shapes = new char[] {"input:1x3x720x1280"};
		trtOptions->trt_dump_ep_context_model = 1;
		trtOptions->trt_ep_context_file_path = new char[] {"trt"};
		options.AppendExecutionProvider_TensorRT_V2(*trtOptions);

		OrtCUDAProviderOptionsV2* cudaOptions;
		ortApi.CreateCUDAProviderOptions(&cudaOptions);

		const char* keys[]{ "device_id", "has_user_compute_stream"};
		std::string deviceIdValue = std::to_string(deviceId);
		const char* values[]{ deviceIdValue.c_str(), "1"};
		ortApi.UpdateCUDAProviderOptions(cudaOptions, keys, values, std::size(keys));

		options.AppendExecutionProvider_CUDA_V2(*cudaOptions);

		_session = Ort::Session(_env, L"model.onnx", options);

		ortApi.ReleaseCUDAProviderOptions(cudaOptions);
		ortApi.ReleaseTensorRTProviderOptions(trtOptions);

		_cudaMemInfo = Ort::MemoryInfo("Cuda", OrtAllocatorType::OrtDeviceAllocator, deviceId, OrtMemTypeDefault);
	} catch (const Ort::Exception& e) {
		OutputDebugStringA(e.what());
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

	try {
		Ort::IoBinding binding(_session);
		const int64_t inputShape[]{ 1,3,_inputSize.cy,_inputSize.cx };
		Ort::Value inputValue = Ort::Value::CreateTensor(
			_cudaMemInfo,
			inputMem,
			inputNumBytes,
			inputShape,
			std::size(inputShape),
			ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT16
		);
		const int64_t outputShape[]{ 1,3,_inputSize.cy * 2,_inputSize.cx * 2 };
		Ort::Value outputValue = Ort::Value::CreateTensor(
			_cudaMemInfo,
			outputMem,
			outputNumBytes,
			outputShape,
			std::size(outputShape),
			ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT16
		);
		binding.BindInput("input", inputValue);
		binding.BindOutput("output", outputValue);

		Ort::RunOptions runOptions;
		runOptions.AddConfigEntry("disable_synchronize_execution_providers", "1");
		_session.Run(runOptions, binding);
	} catch (const Ort::Exception& e) {
		OutputDebugStringA(e.what());
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
