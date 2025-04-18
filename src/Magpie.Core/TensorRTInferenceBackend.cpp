#include "pch.h"
#include "TensorRTInferenceBackend.h"

#ifdef _M_X64

#include "DeviceResources.h"
#include <cuda/cuda_d3d11_interop.h>
#include "shaders/TextureToTensorCS.h"
#include "shaders/TensorToTextureCS.h"
#include "BackendDescriptorStore.h"
#include "Logger.h"
#include "DirectXHelper.h"
#include "Utils.h"
#include "OnnxHelper.h"
#include "HashHelper.h"
#include "Win32Utils.h"
#include "StrUtils.h"
#include "CommonSharedConstants.h"

namespace Magpie::Core {

static void LogCudaError(std::string_view msg, cudaError_t cudaResult) noexcept {
	Logger::Get().Error(fmt::format("{}\n\tCUDA error code: {}", msg, (int)cudaResult));
}

static bool CheckComputeCapability(int deviceId) noexcept {
	int major, minor;

	cudaError_t cudaResult = cudaDeviceGetAttribute(&major, cudaDevAttrComputeCapabilityMajor, deviceId);
	if (cudaResult != cudaError_t::cudaSuccess) {
		Logger::Get().Error("cudaDeviceGetAttribute 失败");
		return false;
	}

	cudaResult = cudaDeviceGetAttribute(&minor, cudaDevAttrComputeCapabilityMinor, deviceId);
	if (cudaResult != cudaError_t::cudaSuccess) {
		Logger::Get().Error("cudaDeviceGetAttribute 失败");
		return false;
	}

	Logger::Get().Info(fmt::format("当前设备 Compute Capability: {}.{}", major, minor));

	// TensorRT 要求 Compute Capability 至少为 6.0
	// https://docs.nvidia.com/deeplearning/tensorrt/support-matrix/index.html
	if (major < 6) {
		Logger::Get().Error("当前设备无法使用 TensorRT");
		return false;
	}

	return true;
}

static std::wstring GetCacheDir(
	const std::vector<uint8_t>& modelData,
	IDXGIAdapter4* adapter,
	std::pair<uint16_t, uint16_t> minShapes,
	std::pair<uint16_t, uint16_t> maxShapes,
	std::pair<uint16_t, uint16_t> optShapes,
	uint8_t optimizationLevel,
	bool enableFP16
) noexcept {
	DXGI_ADAPTER_DESC desc;
	adapter->GetDesc(&desc);

	// TensorRT 缓存和多种因素绑定，这里考虑的因素有：
	// * 模型哈希
	// * ONNX Runtime 版本
	// * TensorRT 版本
	// * 显卡型号 (替代 Compute Capability)
	// * 配置文件
	// * 优化等级
	// * 是否启用半精度
	std::string str = fmt::format(
		"modelHash:{}\nortVersion:{}\nvendorId:{}\ndeviceId:{}\nminShapes:{},{}\nmaxShapes:{},{}\noptShapes:{},{}\noptLevel:{}\nfp16:{}",
		Utils::HashData(modelData), Ort::GetVersionString(), desc.VendorId, desc.DeviceId,
		minShapes.first, minShapes.second, maxShapes.first, maxShapes.second, optShapes.first,
		optShapes.second, optimizationLevel, enableFP16);

	std::wstring strHash = HashHelper::HexHash(std::span((const BYTE*)str.data(), str.size()));
	return StrUtils::Concat(CommonSharedConstants::CACHE_DIR, L"tensorrt\\", strHash);
}

static void* ShareBufferWithCuda(
	const winrt::com_ptr<ID3D11Buffer>& buffer,
	uint32_t bufferSize,
	cudaExternalMemory_t* bufferCudaMem,
	cudaExternalSemaphore_t* bufferCudaSem
) noexcept {
	winrt::com_ptr<IDXGIResource> dxgiRes = buffer.try_as<IDXGIResource>();
	if (!dxgiRes) {
		return nullptr;
	}

	HANDLE sharedHandle = NULL;
	HRESULT hr = dxgiRes->GetSharedHandle(&sharedHandle);
	if (FAILED(hr)) {
		Logger::Get().ComError("GetSharedHandle 失败", hr);
		return nullptr;
	}

	cudaExternalMemoryHandleDesc externalMemoryHandleDesc{
		.type = cudaExternalMemoryHandleTypeD3D11ResourceKmt,
		.handle = {.win32 = {.handle = sharedHandle } },
		.size = bufferSize,
		.flags = cudaExternalMemoryDedicated
	};
	cudaError_t cudaResult = cudaImportExternalMemory(
		bufferCudaMem, &externalMemoryHandleDesc);
	if (cudaResult != cudaError_t::cudaSuccess) {
		LogCudaError("cudaImportExternalMemory 失败", cudaResult);
		return nullptr;
	}

	cudaExternalSemaphoreHandleDesc extSemaDesc{
		.type = cudaExternalSemaphoreHandleTypeKeyedMutexKmt,
		.handle = {.win32 = {.handle = sharedHandle } },
	};
	cudaResult = cudaImportExternalSemaphore(bufferCudaSem, &extSemaDesc);
	if (cudaResult != cudaError_t::cudaSuccess) {
		LogCudaError("cudaImportExternalSemaphore 失败", cudaResult);
		return nullptr;
	}

	void* bufferCudaPtr = nullptr;
	cudaExternalMemoryBufferDesc externalMemoryBufferDesc{ .size = bufferSize };
	cudaResult = cudaExternalMemoryGetMappedBuffer(
		&bufferCudaPtr, *bufferCudaMem, &externalMemoryBufferDesc);
	if (cudaResult != cudaError_t::cudaSuccess) {
		LogCudaError("cudaExternalMemoryGetMappedBuffer 失败", cudaResult);
		return nullptr;
	}

	return bufferCudaPtr;
}

TensorRTInferenceBackend::~TensorRTInferenceBackend() {
	if (_inputBufferCudaSem) {
		cudaDestroyExternalSemaphore((cudaExternalSemaphore_t)_inputBufferCudaSem);
	}
	if (_outputBufferCudaSem) {
		cudaDestroyExternalSemaphore((cudaExternalSemaphore_t)_outputBufferCudaSem);
	}
	if (_inputBufferCudaPtr) {
		cudaFree(_inputBufferCudaPtr);
	}
	if (_outputBufferCudaPtr) {
		cudaFree(_outputBufferCudaPtr);
	}
	if (_inputBufferCudaMem) {
		cudaDestroyExternalMemory((cudaExternalMemory_t)_inputBufferCudaMem);
	}
	if (_outputBufferCudaMem) {
		cudaDestroyExternalMemory((cudaExternalMemory_t)_outputBufferCudaMem);
	}
}

bool TensorRTInferenceBackend::Initialize(
	const wchar_t* modelPath,
	uint32_t scale,
	DeviceResources& deviceResources,
	BackendDescriptorStore& descriptorStore,
	ID3D11Texture2D* input,
	ID3D11Texture2D** output
) noexcept {
	if (!Win32Utils::FileExists(L"third_party\\onnxruntime_providers_tensorrt.dll")) {
		Logger::Get().Error("未安装 TensorRT 拓展");
		return false;
	}

	int deviceId = 0;
	cudaError_t cudaResult = cudaD3D11GetDevice(&deviceId, deviceResources.GetGraphicsAdapter());
	if (cudaResult != cudaError_t::cudaSuccess) {
		LogCudaError("cudaD3D11GetDevice 失败", cudaResult);
		return false;
	}

	if (!CheckComputeCapability(deviceId)) {
		Logger::Get().Error("CheckComputeCapability 失败");
		return false;
	}

	cudaResult = cudaSetDevice(deviceId);
	if (cudaResult != cudaError_t::cudaSuccess) {
		LogCudaError("cudaSetDevice 失败", cudaResult);
		return false;
	}

	bool isFP16Data = false;
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

		if (!_IsModelValid(_session, isFP16Data)) {
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

	const SIZE inputSize = DirectXHelper::GetTextureSize(input);
	const SIZE outputSize = SIZE{ inputSize.cx * (LONG)scale, inputSize.cy * (LONG)scale };

	// 创建输出纹理
	winrt::com_ptr<ID3D11Texture2D> outputTex = DirectXHelper::CreateTexture2D(
		d3dDevice,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		outputSize.cx,
		outputSize.cy,
		D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS
	);
	if (!outputTex) {
		Logger::Get().Error("创建输出纹理失败");
		return false;
	}
	*output = outputTex.get();

	const uint32_t inputElemCount = uint32_t(inputSize.cx * inputSize.cy * 3);
	const uint32_t outputElemCount = uint32_t(outputSize.cx * outputSize.cy * 3);
	const uint32_t inputBufferSize = isFP16Data ? ((inputElemCount + 1) / 2 * 4) : (inputElemCount * 4);
	const uint32_t outputBufferSize = isFP16Data ? ((outputElemCount + 1) / 2 * 4) : (outputElemCount * 4);

	winrt::com_ptr<ID3D11Buffer> inputBuffer;
	winrt::com_ptr<ID3D11Buffer> outputBuffer;
	{
		D3D11_BUFFER_DESC desc{
			.ByteWidth = inputBufferSize,
			.BindFlags = D3D11_BIND_UNORDERED_ACCESS,
			.MiscFlags = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX
		};
		HRESULT hr = d3dDevice->CreateBuffer(&desc, nullptr, inputBuffer.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateBuffer 失败", hr);
			return false;
		}

		desc.ByteWidth = outputBufferSize;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		hr = d3dDevice->CreateBuffer(&desc, nullptr, outputBuffer.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateBuffer 失败", hr);
			return false;
		}
	}

	_inputBufferCudaPtr = ShareBufferWithCuda(
		inputBuffer,
		inputBufferSize,
		(cudaExternalMemory_t*)&_inputBufferCudaMem,
		(cudaExternalSemaphore_t*)&_inputBufferCudaSem
	);
	_outputBufferCudaPtr = ShareBufferWithCuda(
		outputBuffer,
		outputBufferSize,
		(cudaExternalMemory_t*)&_outputBufferCudaMem,
		(cudaExternalSemaphore_t*)&_outputBufferCudaSem
	);
	if (!_inputBufferCudaPtr || !_outputBufferCudaPtr) {
		Logger::Get().Error("ShareBufferWithCuda 失败");
		return false;
	}

	try {
		_ioBinding = Ort::IoBinding(_session);

		const int64_t inputShape[]{ 1,3,inputSize.cy,inputSize.cx };
		_ioBinding.BindInput("input", Ort::Value::CreateTensor(
			_cudaMemInfo,
			_inputBufferCudaPtr,
			inputBufferSize,
			inputShape,
			std::size(inputShape),
			isFP16Data ? ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT16 : ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT
		));

		const int64_t outputShape[]{ 1,3,outputSize.cy,outputSize.cx };
		_ioBinding.BindOutput("output", Ort::Value::CreateTensor(
			_cudaMemInfo,
			_outputBufferCudaPtr,
			outputBufferSize,
			outputShape,
			std::size(outputShape),
			isFP16Data ? ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT16 : ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT
		));
	} catch (const Ort::Exception& e) {
		Logger::Get().Error(e.what());
		return false;
	}

	_inputBufferKmt = inputBuffer.try_as<IDXGIKeyedMutex>();
	if (!_inputBufferKmt) {
		return false;
	}

	_outputBufferKmt = outputBuffer.try_as<IDXGIKeyedMutex>();
	if (!_outputBufferKmt) {
		return false;
	}

	_inputTexSrv = descriptorStore.GetShaderResourceView(input);
	if (!_inputTexSrv) {
		Logger::Get().Error("GetShaderResourceView 失败");
		return false;
	}

	_sampler = deviceResources.GetSampler(
		D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP);
	if (!_sampler) {
		Logger::Get().Error("GetSampler 失败");
		return false;
	}

	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC desc{
			.Format = isFP16Data ? DXGI_FORMAT_R16_FLOAT : DXGI_FORMAT_R32_FLOAT,
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
			.Format = isFP16Data ? DXGI_FORMAT_R16_FLOAT : DXGI_FORMAT_R32_FLOAT,
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
		(inputSize.cx + TEX_TO_TENSOR_BLOCK_SIZE.first - 1) / TEX_TO_TENSOR_BLOCK_SIZE.first,
		(inputSize.cy + TEX_TO_TENSOR_BLOCK_SIZE.second - 1) / TEX_TO_TENSOR_BLOCK_SIZE.second
	};
	_tensorToTexDispatchCount = {
		(outputSize.cx + TENSOR_TO_TEX_BLOCK_SIZE.first - 1) / TENSOR_TO_TEX_BLOCK_SIZE.first,
		(outputSize.cy + TENSOR_TO_TEX_BLOCK_SIZE.second - 1) / TENSOR_TO_TEX_BLOCK_SIZE.second
	};

	return true;
}

void TensorRTInferenceBackend::Evaluate() noexcept {
	// 输入纹理 -> 输入张量
	HRESULT hr = _inputBufferKmt->AcquireSync(_inputBufferMutexKey, INFINITE);
	if (FAILED(hr)) {
		Logger::Get().ComError("AcquireSync 失败", hr);
		return;
	}

	_d3dDC->CSSetShaderResources(0, 1, &_inputTexSrv);
	_d3dDC->CSSetSamplers(0, 1, &_sampler);
	{
		ID3D11UnorderedAccessView* uav = _inputBufferUav.get();
		_d3dDC->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);
	}

	_d3dDC->CSSetShader(_texToTensorShader.get(), nullptr, 0);
	_d3dDC->Dispatch(_texToTensorDispatchCount.first, _texToTensorDispatchCount.second, 1);

	_inputBufferKmt->ReleaseSync(++_inputBufferMutexKey);

	{
		cudaExternalSemaphore_t semArr[] = {
			(cudaExternalSemaphore_t)_inputBufferCudaSem,
			(cudaExternalSemaphore_t)_outputBufferCudaSem
		};
		cudaExternalSemaphoreWaitParams extSemWaitParamsArr[] = {
			{.params{.keyedMutex{.key = _inputBufferMutexKey, .timeoutMs = INFINITE}}},
			{.params{.keyedMutex{.key = _outputBufferMutexKey, .timeoutMs = INFINITE}}}
		};
		cudaError_t cudaResult = cudaWaitExternalSemaphoresAsync(semArr, extSemWaitParamsArr, 2);
		if (cudaResult != cudaError_t::cudaSuccess) {
			LogCudaError("cudaWaitExternalSemaphoresAsync 失败", cudaResult);
			return;
		}
	}
	
	try {
		Ort::RunOptions runOptions;
		runOptions.AddConfigEntry("disable_synchronize_execution_providers", "1");
		_session.Run(runOptions, _ioBinding);
	} catch (const Ort::Exception& e) {
		Logger::Get().Error(e.what());
		return;
	}

	{
		cudaExternalSemaphore_t semArr[] = {
			(cudaExternalSemaphore_t)_inputBufferCudaSem,
			(cudaExternalSemaphore_t)_outputBufferCudaSem
		};

		cudaExternalSemaphoreSignalParams extSemSigParams[] = {
			{.params = {.keyedMutex = {.key = ++_inputBufferMutexKey}}},
			{.params = {.keyedMutex = {.key = ++_outputBufferMutexKey}}}
		};
		cudaError_t cudaResult = cudaSignalExternalSemaphoresAsync(semArr, extSemSigParams, 2);
		if (cudaResult != cudaError_t::cudaSuccess) {
			LogCudaError("cudaSignalExternalSemaphoresAsync 失败", cudaResult);
			return;
		}
	}
	
	// 输出张量 -> 输出纹理
	hr = _outputBufferKmt->AcquireSync(_outputBufferMutexKey, INFINITE);
	if (FAILED(hr)) {
		Logger::Get().ComError("AcquireSync 失败", hr);
		return;
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

	_outputBufferKmt->ReleaseSync(++_outputBufferMutexKey);
}

bool TensorRTInferenceBackend::_CreateSession(
	DeviceResources& deviceResources,
	int deviceId,
	Ort::SessionOptions& sessionOptions,
	const wchar_t* modelPath
) {
	const std::pair<uint16_t, uint16_t> minShapes(uint16_t(1), uint16_t(1));
	const std::pair<uint16_t, uint16_t> maxShapes(uint16_t(1920), uint16_t(1080));
	const std::pair<uint16_t, uint16_t> optShapes(uint16_t(1920), uint16_t(1080));

	const bool enableFP16 = true;
	const uint8_t optimizationLevel = 5;

	std::vector<uint8_t> modelData;
	if (!Win32Utils::ReadFile(modelPath, modelData)) {
		Logger::Get().Error("读取模型失败");
		return false;
	}

	const std::wstring cacheDir = GetCacheDir(
		modelData,
		deviceResources.GetGraphicsAdapter(),
		minShapes,
		maxShapes,
		optShapes,
		optimizationLevel,
		enableFP16
	);
	if (!Win32Utils::CreateDir(cacheDir, true)) {
		Logger::Get().Win32Error("创建缓存文件夹失败");
		return false;
	}

	const std::wstring cacheCtxPath = cacheDir + L"\\ctx.onnx";

	const OrtApi& ortApi = Ort::GetApi();

	OnnxHelper::unique_tensorrt_provider_options trtOptions;
	Ort::ThrowOnError(ortApi.CreateTensorRTProviderOptions(trtOptions.put()));

	const std::string deviceIdStr = std::to_string(deviceId);
	{
		const char* keys[]{
			"device_id",
			"has_user_compute_stream",
			"trt_fp16_enable",
			"trt_builder_optimization_level",
			"trt_profile_min_shapes",
			"trt_profile_max_shapes",
			"trt_profile_opt_shapes",
			"trt_engine_cache_enable",
			"trt_engine_cache_prefix",
			"trt_dump_ep_context_model",
			"trt_ep_context_file_path"
		};
		std::string optLevelStr = std::to_string(optimizationLevel);
		std::string minShapesStr = fmt::format("input:1x3x{}x{}", minShapes.second, minShapes.first);
		std::string maxShapesStr = fmt::format("input:1x3x{}x{}", maxShapes.second, maxShapes.first);
		std::string optShapesStr = fmt::format("input:1x3x{}x{}", optShapes.second, optShapes.first);

		std::string cacheDirANSI = StrUtils::UTF16ToANSI(cacheDir);
		std::string cacheCtxPathANSI = StrUtils::UTF16ToANSI(cacheCtxPath);

		const char* values[]{
			deviceIdStr.c_str(),
			"1",
			enableFP16 ? "1" : "0",
			optLevelStr.c_str(),
			minShapesStr.c_str(),
			maxShapesStr.c_str(),
			optShapesStr.c_str(),
			"1",
			"trt",
			"1",
			cacheCtxPathANSI.c_str()
		};
		Ort::ThrowOnError(ortApi.UpdateTensorRTProviderOptions(trtOptions.get(), keys, values, std::size(keys)));
	}

	OnnxHelper::unique_cuda_provider_options cudaOptions;
	Ort::ThrowOnError(ortApi.CreateCUDAProviderOptions(cudaOptions.put()));

	{
		const char* keys[]{ "device_id", "has_user_compute_stream" };
		const char* values[]{ deviceIdStr.c_str(), "1" };
		Ort::ThrowOnError(ortApi.UpdateCUDAProviderOptions(cudaOptions.get(), keys, values, std::size(keys)));
	}

	sessionOptions.AppendExecutionProvider_TensorRT_V2(*trtOptions.get());
	sessionOptions.AppendExecutionProvider_CUDA_V2(*cudaOptions.get());

	if (Win32Utils::FileExists(cacheCtxPath.c_str())) {
		Logger::Get().Info("读取缓存 " + StrUtils::UTF16ToUTF8(cacheCtxPath));
		_session = Ort::Session(_env, cacheCtxPath.c_str(), sessionOptions);
	} else {
		_session = Ort::Session(_env, modelData.data(), modelData.size(), sessionOptions);
	}

	return true;
}

}

#endif
