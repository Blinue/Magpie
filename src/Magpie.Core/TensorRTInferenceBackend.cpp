#include "pch.h"
#include "TensorRTInferenceBackend.h"
#include "DeviceResources.h"
#include "StrUtils.h"
#include "Win32Utils.h"
#include "Logger.h"
#include "HashHelper.h"
#include "CommonSharedConstants.h"
#include "Utils.h"
#include "OnnxHelper.h"

#pragma warning(push)
// C4100: “pluginFactory”: 未引用的形参
// C4996: 'nvinfer1::IPluginV2' : 被声明为已否决
#pragma warning(disable: 4100 4996)
#include <NvInfer.h>
#pragma warning(pop)

namespace Magpie::Core {

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
		"modelHash:{}\nortVersion:{}\ntrtVersion:{}\nvendorId:{}\ndeviceId:{}\nminShapes:{},{}\nmaxShapes:{},{}\noptShapes:{},{}\noptLevel:{}\nfp16:{}",
		Utils::HashData(modelData), Ort::GetVersionString(), NV_TENSORRT_VERSION, desc.VendorId, desc.DeviceId,
		minShapes.first, minShapes.second, maxShapes.first, maxShapes.second, optShapes.first,
		optShapes.second, optimizationLevel, enableFP16);

	std::wstring strHash = HashHelper::HexHash(std::span((const BYTE*)str.data(), str.size()));
	return StrUtils::Concat(CommonSharedConstants::CACHE_DIR, L"tensorrt\\", strHash);
}

bool TensorRTInferenceBackend::_CheckComputeCapability(int deviceId) noexcept {
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
