#include "pch.h"
#include "TensorRTInferenceBackend.h"
#include "DeviceResources.h"
#include <cuda_d3d11_interop.h>
#include "shaders/TextureToTensorCS.h"
#include "shaders/TensorToTextureCS.h"
#include "BackendDescriptorStore.h"
#include "Logger.h"
#include "OnnxStatus.h"
#include "DirectXHelper.h"
#include "OnnxHelper.h"
#include "OnnxEffectDrawer.h"
#include "HashHelper.h"
#include "Win32Helper.h"
#include "StrHelper.h"
#include "ScalingWindow.h"
#include "ScalingOptions.h"
#include "CommonSharedConstants.h"

#pragma warning(push)
// C4100: “pluginFactory”: 未引用的形参
// C4996: 'nvinfer1::IPluginV2' : 被声明为已否决
#pragma warning(disable: 4100 4996)
#include <NvInfer.h>
#pragma warning(pop)

namespace Magpie {

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

static std::wstring ModelStem(const wchar_t* modelPath) noexcept {
	std::wstring stem(modelPath);
	if (size_t slash = stem.find_last_of(L"\\\\/"); slash != std::wstring::npos) {
		stem.erase(0, slash + 1);
	}
	if (size_t dot = stem.find_last_of(L'.'); dot != std::wstring::npos) {
		stem.erase(dot);
	}
	return stem;
}

static std::wstring GetCacheDir(
	const std::vector<uint8_t>& modelData,
	const wchar_t* modelPath,
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
		"modelHash:{}\nortVersion:{}\ntrtVersion:{}\nvendorId:{}\ndeviceId:{}\nminShapes:{},{}\nmaxShapes:{},{}\noptShapes:{},{}\noptLevel:{}\nfp16:{}",
		HashHelper::Hash64(modelData), Ort::GetVersionString(), NV_TENSORRT_VERSION, desc.VendorId, desc.DeviceId,
		minShapes.first, minShapes.second, maxShapes.first, maxShapes.second, optShapes.first,
		optShapes.second, optimizationLevel, enableFP16);

	std::wstring strHash = HashHelper::HexHash(std::span((const BYTE*)str.data(), str.size()));
	// 目录名带上模型名和分辨率，便于识别 / name the folder after the model and the
	// profile size so the cache is identifiable at a glance instead of a bare hash.
	const std::wstring stem = ModelStem(modelPath);

	std::wstring dirName(stem);
	dirName += L'_';
	dirName += std::to_wstring(maxShapes.first);
	dirName += L'x';
	dirName += std::to_wstring(maxShapes.second);
	dirName += L'_';
	dirName += strHash;

	return StrHelper::Concat(CommonSharedConstants::CACHE_DIR, L"tensorrt\\", dirName);
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
	if (!Win32Helper::FileExists(L"third_party\\onnxruntime_providers_tensorrt.dll")) {
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

	// TensorRT bakes an optimization profile into the engine at build time and
	// rejects any input outside it. Upstream hardcoded a 1920x1080 maximum, so a
	// larger source failed every frame with "does not satisfy any optimization
	// profiles" and the user just saw a black screen. Derive the profile from the
	// actual input instead - GetCacheDir already hashes these shapes, so each
	// resolution simply gets its own cached engine.
	const SIZE inputSize = OnnxHelper::GetTextureSize(input);

	bool isFP16Data = false;
	try {
		const OrtApi& ortApi = Ort::GetApi();

		_env = Ort::Env(ORT_LOGGING_LEVEL_INFO, "", _OrtLog, nullptr);

		Ort::SessionOptions sessionOptions;
		sessionOptions.SetIntraOpNumThreads(1);

		Ort::ThrowOnError(ortApi.AddFreeDimensionOverride(sessionOptions, "DATA_BATCH", 1));

		if (!_CreateSession(deviceResources, deviceId, sessionOptions, modelPath,
			uint32_t(inputSize.cx), uint32_t(inputSize.cy),
			ScalingWindow::Get().Options().onnxStaticEngine != 0,
			ScalingWindow::Get().Options().onnxDynamicMaxWidth,
			ScalingWindow::Get().Options().onnxDynamicMaxHeight,
			ScalingWindow::Get().Options().onnxDynamicMinWidth,
			ScalingWindow::Get().Options().onnxDynamicMinHeight)) {
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
	if (_evaluateFailed) {
		// 已失败：停止推理，避免刷屏。注意输出纹理仍在链上，需重新开始缩放
		// Latched: stop inferring so a broken session does not spam the log every
		// frame. The model's output texture is still wired into the chain, so the
		// picture does not recover here - scaling must be restarted.
		return;
	}

	// 输入纹理 -> 输入张量
	HRESULT hr = _inputBufferKmt->AcquireSync(_inputBufferMutexKey, INFINITE);
	if (FAILED(hr)) {
		Logger::Get().ComError("AcquireSync 失败", hr);
		_OnEvaluateFailed();
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
			_OnEvaluateFailed();
			return;
		}
	}
	
	try {
		Ort::RunOptions runOptions;
		runOptions.AddConfigEntry("disable_synchronize_execution_providers", "1");
		_session.Run(runOptions, _ioBinding);
	} catch (const Ort::Exception& e) {
		Logger::Get().Error(e.what());
		_OnEvaluateFailed();
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
			_OnEvaluateFailed();
			return;
		}
	}
	
	// 输出张量 -> 输出纹理

	hr = _outputBufferKmt->AcquireSync(_outputBufferMutexKey, INFINITE);
	if (FAILED(hr)) {
		Logger::Get().ComError("AcquireSync 失败", hr);
		_OnEvaluateFailed();
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

void TensorRTInferenceBackend::_OnEvaluateFailed() noexcept {
	_evaluateFailed = true;

	// 清掉非粘滞的 CUDA 错误，让下一次缩放从干净状态开始
	// Clear non-sticky CUDA errors so the next scale starts clean. A sticky
	// error (illegal address, launch failure) poisons the context for the
	// lifetime of the process and genuinely does need a restart.
	cudaGetLastError();

	Logger::Get().Error(
		"推理失败，本次缩放禁用 AI / inference failed - AI upscaling is disabled "
		"for this scaling session. Stop and start scaling to retry; if it keeps "
		"failing, restart Magpie to reset the CUDA context.");

	OnnxStatus::Report(L"AI upscaling stopped",
		L"Inference failed, so scaling continues without the model. Restart "
		L"scaling to retry.");
}

bool TensorRTInferenceBackend::_CreateSession(
	DeviceResources& deviceResources,
	int deviceId,
	Ort::SessionOptions& sessionOptions,
	const wchar_t* modelPath,
	uint32_t inputWidth,
	uint32_t inputHeight,
	bool staticEngine,
	uint32_t dynamicMaxWidth,
	uint32_t dynamicMaxHeight,
	uint32_t dynamicMinWidth,
	uint32_t dynamicMinHeight
) {
	// TensorRT profiles are a range (min..max): an engine built for 1440p serves
	// any smaller window, but not a larger one. So round the input up to the next
	// standard tier, and if a BIGGER engine for this model is already cached,
	// reuse its dimensions so we get a cache hit instead of building again.
	static constexpr uint32_t TIERS[][2] = {
		{1280, 720}, {1920, 1080}, {2560, 1440}, {3200, 1800},
		{3840, 2160}, {5120, 2880}, {7680, 4320}
	};
	uint32_t profileWidth = 0;
	uint32_t profileHeight = 0;
	for (const auto& tier : TIERS) {
		if (tier[0] >= inputWidth && tier[1] >= inputHeight) {
			profileWidth = tier[0];
			profileHeight = tier[1];
			break;
		}
	}
	if (profileWidth == 0) {
		profileWidth = std::min(inputWidth, 65535u);
		profileHeight = std::min(inputHeight, 65535u);
	}

	{
		// cache dir names are <stem>_<W>x<H>_<hash>
		const std::wstring stem = ModelStem(modelPath);
		const std::wstring pattern =
			StrHelper::Concat(CommonSharedConstants::CACHE_DIR, L"tensorrt\\", stem, L"_*");
		uint32_t bestW = 0;
		uint32_t bestH = 0;
		WIN32_FIND_DATAW fd{};
		HANDLE hFind = FindFirstFileW(pattern.c_str(), &fd);
		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
					continue;
				}
				const std::wstring name(fd.cFileName);
				const size_t lastU = name.rfind(L'_');
				if (lastU == std::wstring::npos || lastU == 0) {
					continue;
				}
				const size_t prevU = name.rfind(L'_', lastU - 1);
				if (prevU == std::wstring::npos) {
					continue;
				}
				const std::wstring res = name.substr(prevU + 1, lastU - prevU - 1);
				const size_t xPos = res.find(L'x');
				if (xPos == std::wstring::npos) {
					continue;
				}
				const uint32_t w = (uint32_t)_wtoi(res.substr(0, xPos).c_str());
				const uint32_t h = (uint32_t)_wtoi(res.substr(xPos + 1).c_str());
				if (w < inputWidth || h < inputHeight) {
					continue;
				}
				if (bestW == 0 || (uint64_t)w * h < (uint64_t)bestW * bestH) {
					bestW = w;
					bestH = h;
				}
			} while (FindNextFileW(hFind, &fd));
			FindClose(hFind);
		}
		if (bestW != 0) {
			profileWidth = bestW;
			profileHeight = bestH;
		}
	}

	// 用户指定了动态引擎的上限则优先使用 / an explicit max wins over the tier
	// search: the user knows the largest window they will actually scale.
	if (!staticEngine && dynamicMaxWidth != 0 && dynamicMaxHeight != 0) {
		profileWidth = std::clamp(std::max(dynamicMaxWidth, inputWidth), 1u, 65535u);
		profileHeight = std::clamp(std::max(dynamicMaxHeight, inputHeight), 1u, 65535u);
	}

	if (staticEngine) {
		// 静态引擎：min=opt=max，最快但只适用于该分辨率
		// Static: min=opt=max. Fastest, because TensorRT tunes kernels for this
		// exact size - but the engine is only valid at it, so each new window
		// size builds its own.
		profileWidth = std::min(inputWidth, 65535u);
		profileHeight = std::min(inputHeight, 65535u);
	}

	// 动态引擎的下限由用户指定，0 表示 1x1
	// Lower bound of a dynamic profile. 0 means 1x1. Never let it exceed the
	// current input or the max, or TensorRT rejects the profile outright.
	uint32_t minWidth = dynamicMinWidth == 0 ? 1u : dynamicMinWidth;
	uint32_t minHeight = dynamicMinHeight == 0 ? 1u : dynamicMinHeight;
	minWidth = std::clamp(minWidth, 1u, std::min(inputWidth, profileWidth));
	minHeight = std::clamp(minHeight, 1u, std::min(inputHeight, profileHeight));

	const std::pair<uint16_t, uint16_t> minShapes = staticEngine
		? std::pair<uint16_t, uint16_t>{ uint16_t(profileWidth), uint16_t(profileHeight) }
		: std::pair<uint16_t, uint16_t>{ uint16_t(minWidth), uint16_t(minHeight) };
	const std::pair<uint16_t, uint16_t> maxShapes{ uint16_t(profileWidth), uint16_t(profileHeight) };
	const std::pair<uint16_t, uint16_t> optShapes{ uint16_t(profileWidth), uint16_t(profileHeight) };

	const bool enableFP16 = true;
	const uint8_t optimizationLevel = 5;

	std::vector<uint8_t> modelData;
	if (!Win32Helper::ReadFile(modelPath, modelData)) {
		Logger::Get().Error("读取模型失败");
		return false;
	}

	const std::wstring cacheDir = GetCacheDir(
		modelData,
		modelPath,
		deviceResources.GetGraphicsAdapter(),
		minShapes,
		maxShapes,
		optShapes,
		optimizationLevel,
		enableFP16
	);
	if (!Win32Helper::CreateDir(cacheDir, true)) {
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

		std::string cacheDirANSI = StrHelper::UTF16ToANSI(cacheDir);
		std::string cacheCtxPathANSI = StrHelper::UTF16ToANSI(cacheCtxPath);

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

	// Building an engine blocks this thread for minutes with no other feedback,
	// which is indistinguishable from a hang. Say so explicitly, and time both
	// paths so a cache hit vs a rebuild is obvious after the fact.
	const bool engineCached = Win32Helper::FileExists(cacheCtxPath.c_str());
	Logger::Get().Info(fmt::format(
		"TensorRT session: input {}x{}, profile {}x{}, fp16={}, optLevel={}, engine cache {}",
		inputWidth, inputHeight, profileWidth, profileHeight,
		enableFP16, uint32_t(optimizationLevel), engineCached ? "HIT" : "MISS"));

	const uint64_t startTick = GetTickCount64();
	if (engineCached) {
		Logger::Get().Info("读取缓存 " + StrHelper::UTF16ToUTF8(cacheCtxPath));
		_session = Ort::Session(_env, cacheCtxPath.c_str(), sessionOptions);
	} else {
		Logger::Get().Info(
			"No cached TensorRT engine for this model at this resolution. Building one "
			"now - this takes minutes and Magpie will appear frozen until it finishes. "
			"Avoid GPU-heavy work meanwhile. It is cached afterwards and reused.");
		OnnxStatus::Report(L"Building AI engine",
			L"First run at this resolution. This takes a few minutes and Magpie "
			L"will look frozen until it finishes.");
		_session = Ort::Session(_env, modelData.data(), modelData.size(), sessionOptions);
	}
	if (!engineCached) {
		OnnxStatus::Report(L"AI engine ready",
			L"The engine is built and cached; later scales start instantly.");
	}
	Logger::Get().Info(fmt::format("TensorRT engine {} in {} ms",
		engineCached ? "loaded from cache" : "BUILT", GetTickCount64() - startTick));

	return true;
}

}
