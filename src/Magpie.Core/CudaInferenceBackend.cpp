#include "pch.h"
#include "CudaInferenceBackend.h"
#include "DeviceResources.h"
#include <cuda_d3d11_interop.h>
#include "shaders/TextureToTensorCS.h"
#include "shaders/TensorToTextureCS.h"
#include "BackendDescriptorStore.h"
#include "Logger.h"
#include "DirectXHelper.h"
#include "Utils.h"

#pragma comment(lib, "cudart.lib")

namespace Magpie::Core {

static void LogCudaError(std::string_view msg, cudaError_t cudaResult) noexcept {
	Logger::Get().Error(fmt::format("{}\n\tCUDA error code: {}", msg, (int)cudaResult));
}

CudaInferenceBackend::~CudaInferenceBackend() {
	if (_inputBufferCuda) {
		cudaGraphicsUnregisterResource(_inputBufferCuda);
	}
	if (_outputBufferCuda) {
		cudaGraphicsUnregisterResource(_outputBufferCuda);
	}
}

bool CudaInferenceBackend::Initialize(
	const wchar_t* modelPath,
	uint32_t scale,
	DeviceResources& deviceResources,
	BackendDescriptorStore& descriptorStore,
	ID3D11Texture2D* input,
	ID3D11Texture2D** output
) noexcept {
	int deviceId = 0;
	cudaError_t cudaResult = cudaD3D11GetDevice(&deviceId, deviceResources.GetGraphicsAdapter());
	if (cudaResult != cudaError_t::cudaSuccess) {
		LogCudaError("cudaD3D11GetDevice 失败", cudaResult);
		return false;
	}

	if (!_CheckComputeCapability(deviceId)) {
		Logger::Get().Error("CheckComputeCapability 失败");
		return false;
	}

	cudaResult = cudaSetDevice(deviceId);
	if (cudaResult != cudaError_t::cudaSuccess) {
		LogCudaError("cudaSetDevice 失败", cudaResult);
		return false;
	}

	try {
		const OrtApi& ortApi = Ort::GetApi();

		_env = Ort::Env(ORT_LOGGING_LEVEL_INFO, "", _OrtLog, nullptr);

		Ort::SessionOptions sessionOptions;
		sessionOptions.SetIntraOpNumThreads(1);

		Ort::ThrowOnError(ortApi.AddFreeDimensionOverride(sessionOptions, "DATA_BATCH", 1));

		if (!_CreateSession(deviceResources, deviceId, sessionOptions, modelPath)) {
			Logger::Get().Error("_CreateSession 失败");
			return false;
		}

		if (!_IsModelValid(_session, _isFP16Data)) {
			Logger::Get().Error("不支持此模型");
			return false;
		}

		_cudaMemInfo = Ort::MemoryInfo("Cuda", OrtAllocatorType::OrtDeviceAllocator, deviceId, OrtMemTypeDefault);
	} catch (const Ort::Exception& e) {
		Logger::Get().Error(e.what());
		return false;
	}

	ID3D11Device5* d3dDevice = deviceResources.GetD3DDevice();
	_d3dDC = deviceResources.GetD3DDC();

	_inputSize = DirectXHelper::GetTextureSize(input);
	_outputSize = SIZE{ _inputSize.cx * (LONG)scale, _inputSize.cy * (LONG)scale };

	// 创建输出纹理
	winrt::com_ptr<ID3D11Texture2D> outputTex = DirectXHelper::CreateTexture2D(
		d3dDevice,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		_outputSize.cx,
		_outputSize.cy,
		D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS
	);
	if (!outputTex) {
		Logger::Get().Error("创建输出纹理失败");
		return false;
	}
	*output = outputTex.get();

	const uint32_t inputElemCount = uint32_t(_inputSize.cx * _inputSize.cy * 3);
	const uint32_t outputElemCount = uint32_t(_outputSize.cx * _outputSize.cy * 3);

	winrt::com_ptr<ID3D11Buffer> inputBuffer;
	winrt::com_ptr<ID3D11Buffer> outputBuffer;
	{
		D3D11_BUFFER_DESC desc{
			.ByteWidth = _isFP16Data ? ((inputElemCount + 1) / 2 * 4) : (inputElemCount * 4),
			.BindFlags = D3D11_BIND_UNORDERED_ACCESS
		};
		HRESULT hr = d3dDevice->CreateBuffer(&desc, nullptr, inputBuffer.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateBuffer 失败", hr);
			return false;
		}

		desc.ByteWidth = _isFP16Data ? ((outputElemCount + 1) / 2 * 4) : (outputElemCount * 4);
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		hr = d3dDevice->CreateBuffer(&desc, nullptr, outputBuffer.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateBuffer 失败", hr);
			return false;
		}
	}

	_inputTexSrv = descriptorStore.GetShaderResourceView(input);
	if (!_inputTexSrv) {
		Logger::Get().Error("GetShaderResourceView 失败");
		return false;
	}

	_pointSampler = deviceResources.GetSampler(
		D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP);
	if (!_pointSampler) {
		Logger::Get().Error("GetSampler 失败");
		return false;
	}

	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC desc{
			.Format = _isFP16Data ? DXGI_FORMAT_R16_FLOAT : DXGI_FORMAT_R32_FLOAT,
			.ViewDimension = D3D11_UAV_DIMENSION_BUFFER,
			.Buffer{
				.NumElements = inputElemCount
			}
		};

		HRESULT hr = d3dDevice->CreateUnorderedAccessView(
			inputBuffer.get(), &desc, _inputBufferUav.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateUnorderedAccessView 失败", hr);
			return false;
		}
	}

	{
		D3D11_SHADER_RESOURCE_VIEW_DESC desc{
			.Format = _isFP16Data ? DXGI_FORMAT_R16_FLOAT : DXGI_FORMAT_R32_FLOAT,
			.ViewDimension = D3D11_SRV_DIMENSION_BUFFER,
			.Buffer{
				.NumElements = outputElemCount
			}
		};

		HRESULT hr = d3dDevice->CreateShaderResourceView(
			outputBuffer.get(), &desc, _outputBufferSrv.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateShaderResourceView 失败", hr);
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
			Logger::Get().ComError("CreateUnorderedAccessView 失败", hr);
			return false;
		}
	}

	HRESULT hr = d3dDevice->CreateComputeShader(
		TextureToTensorCS, sizeof(TextureToTensorCS), nullptr, _texToTensorShader.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateComputeShader 失败", hr);
		return false;
	}

	hr = d3dDevice->CreateComputeShader(
		TensorToTextureCS, sizeof(TensorToTextureCS), nullptr, _tensorToTexShader.put());
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
		(_outputSize.cx + TENSOR_TO_TEX_BLOCK_SIZE.first - 1) / TENSOR_TO_TEX_BLOCK_SIZE.first,
		(_outputSize.cy + TENSOR_TO_TEX_BLOCK_SIZE.second - 1) / TENSOR_TO_TEX_BLOCK_SIZE.second
	};

	cudaResult = cudaGraphicsD3D11RegisterResource(
		&_inputBufferCuda, inputBuffer.get(), cudaGraphicsRegisterFlagsNone);
	if (cudaResult != cudaError_t::cudaSuccess) {
		LogCudaError("cudaGraphicsD3D11RegisterResource 失败", cudaResult);
		return false;
	}

	cudaGraphicsResourceSetMapFlags(_inputBufferCuda, cudaGraphicsMapFlagsReadOnly);

	cudaResult = cudaGraphicsD3D11RegisterResource(
		&_outputBufferCuda, outputBuffer.get(), cudaGraphicsRegisterFlagsNone);
	if (cudaResult != cudaError_t::cudaSuccess) {
		LogCudaError("cudaGraphicsD3D11RegisterResource 失败", cudaResult);
		return false;
	}

	cudaGraphicsResourceSetMapFlags(_outputBufferCuda, cudaGraphicsMapFlagsWriteDiscard);

	return true;
}

void CudaInferenceBackend::Evaluate() noexcept {
	// 输入纹理 -> 输入张量
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
			LogCudaError("cudaGraphicsMapResources 失败", cudaResult);
			return;
		}
	}

	void* inputMem = nullptr;
	size_t inputNumBytes;
	cudaError_t cudaResult = cudaGraphicsResourceGetMappedPointer(&inputMem, &inputNumBytes, _inputBufferCuda);
	if (cudaResult != cudaError_t::cudaSuccess) {
		LogCudaError("cudaGraphicsResourceGetMappedPointer 失败", cudaResult);
		return;
	}

	void* outputMem = nullptr;
	size_t outputNumBytes;
	cudaResult = cudaGraphicsResourceGetMappedPointer(&outputMem, &outputNumBytes, _outputBufferCuda);
	if (cudaResult != cudaError_t::cudaSuccess) {
		LogCudaError("cudaGraphicsResourceGetMappedPointer 失败", cudaResult);
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
			_isFP16Data ? ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT16 : ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT
		);
		const int64_t outputShape[]{ 1,3,_outputSize.cy,_outputSize.cx };
		Ort::Value outputValue = Ort::Value::CreateTensor(
			_cudaMemInfo,
			outputMem,
			outputNumBytes,
			outputShape,
			std::size(outputShape),
			_isFP16Data ? ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT16 : ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT
		);
		binding.BindInput("input", inputValue);
		binding.BindOutput("output", outputValue);

		Ort::RunOptions runOptions;
		runOptions.AddConfigEntry("disable_synchronize_execution_providers", "1");
		_session.Run(runOptions, binding);
	} catch (const Ort::Exception& e) {
		Logger::Get().Error(e.what());
		return;
	}

	{
		cudaGraphicsResource* buffers[] = { _inputBufferCuda, _outputBufferCuda };
		cudaResult = cudaGraphicsUnmapResources(2, buffers);
		if (cudaResult != cudaError_t::cudaSuccess) {
			LogCudaError("cudaGraphicsUnmapResources 失败", cudaResult);
			return;
		}
	}

	// 输出张量 -> 输出纹理
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

bool CudaInferenceBackend::_CreateSession(
	DeviceResources& /*deviceResources*/,
	int deviceId,
	Ort::SessionOptions& sessionOptions,
	const wchar_t* modelPath
) {
	const OrtApi& ortApi = Ort::GetApi();

	OrtCUDAProviderOptionsV2* cudaOptions;
	Ort::ThrowOnError(ortApi.CreateCUDAProviderOptions(&cudaOptions));

	Utils::ScopeExit se([cudaOptions]() {
		Ort::GetApi().ReleaseCUDAProviderOptions(cudaOptions);
	});

	{
		const char* keys[]{ "device_id", "has_user_compute_stream" };
		std::string deviceIdStr = std::to_string(deviceId);
		const char* values[]{ deviceIdStr.c_str(), "1" };
		Ort::ThrowOnError(ortApi.UpdateCUDAProviderOptions(cudaOptions, keys, values, std::size(keys)));
	}

	sessionOptions.AppendExecutionProvider_CUDA_V2(*cudaOptions);

	_session = Ort::Session(_env, modelPath, sessionOptions);
	return true;
}

}
