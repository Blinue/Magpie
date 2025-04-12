#include "pch.h"
#include "OnnxEffectDrawer.h"
#include "Logger.h"
#include "DirectMLInferenceBackend.h"
#include "TensorRTInferenceBackend.h"
#include "Win32Utils.h"
#include <rapidjson/document.h>
#include "StrUtils.h"

namespace Magpie::Core {

OnnxEffectDrawer::OnnxEffectDrawer() {}

OnnxEffectDrawer::~OnnxEffectDrawer() {}

static bool ReadJson(
	const rapidjson::Document& doc,
	std::string& modelPath,
	uint32_t& scale,
	std::string& backend
) noexcept {
	if (!doc.IsObject()) {
		Logger::Get().Error("根元素不是 Object");
		return false;
	}

	auto root = ((const rapidjson::Document&)doc).GetObj();

	{
		auto node = root.FindMember("path");
		if (node == root.MemberEnd() || !node->value.IsString()) {
			Logger::Get().Error("解析 path 失败");
			return false;
		}

		modelPath = node->value.GetString();
	}
	
	{
		auto node = root.FindMember("scale");
		if (node == root.MemberEnd() || !node->value.IsUint()) {
			Logger::Get().Error("解析 scale 失败");
			return false;
		}

		scale = node->value.GetUint();
	}

	{
		auto node = root.FindMember("backend");
		if (node == root.MemberEnd() || !node->value.IsString()) {
			Logger::Get().Error("解析 backend 失败");
			return false;
		}

		backend = node->value.GetString();
	}

	return true;
}

bool OnnxEffectDrawer::Initialize(
	DeviceResources& deviceResources,
	BackendDescriptorStore& descriptorStore,
	ID3D11Texture2D** inOutTexture
) noexcept {
	const wchar_t* jsonPath = L"model.json";
	if (!Win32Utils::FileExists(jsonPath)) {
		return true;
	}
	
	std::string json;
	if (!Win32Utils::ReadTextFile(jsonPath, json)) {
		Logger::Get().Error("Win32Utils::ReadTextFile 失败");
		return false;
	}

	std::string modelPath;
	uint32_t scale = 1;
	std::string backend;
	{
		rapidjson::Document doc;
		doc.ParseInsitu(json.data());
		if (doc.HasParseError()) {
			Logger::Get().Error("解析 json 失败");
			return false;
		}
		
		if (!ReadJson(doc, modelPath, scale, backend)) {
			Logger::Get().Error("ReadJson 失败");
			return false;
		}
	}
	
	StrUtils::ToLowerCase(backend);
	if (backend == "directml" || backend == "dml" || backend == "d") {
		_inferenceBackend = std::make_unique<DirectMLInferenceBackend>();
	}
#if _M_X64
	else if (backend == "tensorrt" || backend == "trt" || backend == "t") {
		_inferenceBackend = std::make_unique<TensorRTInferenceBackend>();
	}
#endif
	else {
		Logger::Get().Error("未知 backend");
		return false;
	}

	std::wstring modelPathW = StrUtils::UTF8ToUTF16(modelPath);
	if (!_inferenceBackend->Initialize(modelPathW.c_str(), scale, deviceResources, descriptorStore, *inOutTexture, inOutTexture)) {
		return false;
	}

	return true;
}

void OnnxEffectDrawer::Draw(EffectsProfiler& /*profiler*/) const noexcept {
	if (_inferenceBackend) {
		_inferenceBackend->Evaluate();
	}
}

}
