#include "pch.h"
#include "ScalingModesService.h"
#include "AppSettings.h"
#include "StrUtils.h"
#include "JsonHelper.h"
#include "EffectsService.h"
#include "EffectHelper.h"

using namespace Magpie::Core;


namespace winrt::Magpie::UI {

ScalingMode& ScalingModesService::GetScalingMode(uint32_t idx) {
	return AppSettings::Get().ScalingModes()[idx];
}

uint32_t ScalingModesService::GetScalingModeCount() {
	return (uint32_t)AppSettings::Get().ScalingModes().size();
}

void ScalingModesService::AddScalingMode(std::wstring_view name, int copyFrom) {
	assert(!name.empty());

	std::vector<ScalingMode>& scalingModes = AppSettings::Get().ScalingModes();
	if (copyFrom < 0) {
		scalingModes.emplace_back().name = name;
	} else {
		scalingModes.emplace_back(scalingModes[copyFrom]).name = name;
	}

	_scalingModeAddedEvent();
}

static void UpdateScalingProfileAfterRemove(ScalingProfile& profile, int removedIdx) {
	if (profile.scalingMode == removedIdx) {
		profile.scalingMode = -1;
	} else if (profile.scalingMode > removedIdx) {
		--profile.scalingMode;
	}
}

void ScalingModesService::RemoveScalingMode(uint32_t index) {
	std::vector<ScalingMode>& scalingModes = AppSettings::Get().ScalingModes();
	scalingModes.erase(scalingModes.begin() + index);

	UpdateScalingProfileAfterRemove(AppSettings::Get().DefaultScalingProfile(), (int)index);
	for (ScalingProfile& profile : AppSettings::Get().ScalingProfiles()) {
		UpdateScalingProfileAfterRemove(profile, (int)index);
	}

	_scalingModeRemovedEvent(index);
}

static void UpdateScalingProfileAfterMove(ScalingProfile& profile, int idx, int targetIdx) {
	if (profile.scalingMode == idx) {
		profile.scalingMode = targetIdx;
	} else if (profile.scalingMode == targetIdx) {
		profile.scalingMode = idx;
	}
}

bool ScalingModesService::MoveScalingMode(uint32_t scalingModeIdx, bool isMoveUp) {
	std::vector<ScalingMode>& profiles = AppSettings::Get().ScalingModes();
	if (isMoveUp ? scalingModeIdx == 0 : scalingModeIdx + 1 >= (uint32_t)profiles.size()) {
		return false;
	}

	int targetIdx = isMoveUp ? (int)scalingModeIdx - 1 : (int)scalingModeIdx + 1;
	std::swap(profiles[scalingModeIdx], profiles[targetIdx]);

	UpdateScalingProfileAfterMove(
		AppSettings::Get().DefaultScalingProfile(),
		(int)scalingModeIdx,
		targetIdx
	);
	for (ScalingProfile& profile : AppSettings::Get().ScalingProfiles()) {
		UpdateScalingProfileAfterMove(profile, (int)scalingModeIdx, targetIdx);
	}

	_scalingModeMovedEvent(scalingModeIdx, isMoveUp);
	return true;
}

static void WriteScalingMode(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer, const ScalingMode& scaleMode) {
	writer.StartObject();
	writer.Key("name");
	writer.String(StrUtils::UTF16ToUTF8(scaleMode.name).c_str());
	if (!scaleMode.effects.empty()) {
		writer.Key("effects");
		writer.StartArray();
		for (const auto& effect : scaleMode.effects) {
			writer.StartObject();
			writer.Key("name");
			writer.String(StrUtils::UTF16ToUTF8(effect.name).c_str());

			if (effect.HasScale()) {
				writer.Key("scalingType");
				writer.Uint((uint32_t)effect.scalingType);
				writer.Key("scale");
				writer.StartObject();
				writer.Key("x");
				writer.Double(effect.scale.first);
				writer.Key("y");
				writer.Double(effect.scale.second);
				writer.EndObject();
			}

			if (!effect.parameters.empty()) {
				writer.Key("parameters");
				writer.StartObject();
				for (const auto& [name, value] : effect.parameters) {
					writer.Key(StrUtils::UTF16ToUTF8(name).c_str());
					writer.Double(value);
				}
				writer.EndObject();
			}

			writer.EndObject();
		}
		writer.EndArray();
	}
	writer.EndObject();
}

void ScalingModesService::Export(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const noexcept {
	writer.Key("scalingModes");
	writer.StartArray();

	for (const ScalingMode& scalingMode : AppSettings::Get().ScalingModes()) {
		WriteScalingMode(writer, scalingMode);
	}

	writer.EndArray();
}

static bool LoadScalingMode(const rapidjson::GenericObject<true, rapidjson::Value>& scalingModeObj, ScalingMode& scalingMode) {
	if (!JsonHelper::ReadString(scalingModeObj, "name", scalingMode.name)) {
		return false;
	}

	auto effectsNode = scalingModeObj.FindMember("effects");
	if (effectsNode == scalingModeObj.MemberEnd()) {
		return true;
	}

	if (!effectsNode->value.IsArray()) {
		return false;
	}

	auto effectsArray = effectsNode->value.GetArray();
	rapidjson::SizeType size = effectsArray.Size();
	scalingMode.effects.resize(size);

	for (rapidjson::SizeType i = 0; i < size; ++i) {
		if (!effectsArray[i].IsObject()) {
			return false;
		}

		auto elemObj = effectsArray[i].GetObj();
		EffectOption& effect = scalingMode.effects[i];

		if (!JsonHelper::ReadString(elemObj, "name", effect.name)) {
			return false;
		}

		if (!JsonHelper::ReadUInt(elemObj, "scalingType", (uint32_t&)effect.scalingType)) {
			return false;
		}

		{
			auto scaleNode = elemObj.FindMember("scale");
			if (scaleNode != elemObj.MemberEnd()) {
				if (!scaleNode->value.IsObject()) {
					return false;
				}

				auto scaleObj = scaleNode->value.GetObj();

				if (!JsonHelper::ReadFloat(scaleObj, "x", effect.scale.first, true)
					|| !JsonHelper::ReadFloat(scaleObj, "y", effect.scale.second, true)
					) {
					return false;
				}
			}
		}

		{
			auto parametersNode = elemObj.FindMember("parameters");
			if (parametersNode != elemObj.MemberEnd()) {
				if (!parametersNode->value.IsObject()) {
					return false;
				}

				auto paramsObj = parametersNode->value.GetObj();
				effect.parameters.reserve(paramsObj.MemberCount());
				for (auto& param : paramsObj) {
					std::wstring name = StrUtils::UTF8ToUTF16(param.name.GetString());

					if (!param.value.IsNumber()) {
						return false;
					}
					effect.parameters[name] = param.value.GetFloat();
				}
			}
		}
	}

	return true;
}

bool ScalingModesService::Import(const rapidjson::GenericObject<true, rapidjson::Value>& root) noexcept {
	auto scalingModesNode = root.FindMember("scalingModes");
	if (scalingModesNode == root.MemberEnd()) {
		return true;
	}

	if (!scalingModesNode->value.IsArray()) {
		return false;
	}

	const auto& scalingModesArray = scalingModesNode->value.GetArray();
	const rapidjson::SizeType size = scalingModesArray.Size();
	if (size == 0) {
		return true;
	}

	std::vector<ScalingMode> scalingModes;
	scalingModes.resize(size);

	for (rapidjson::SizeType i = 0; i < size; ++i) {
		if (!scalingModesArray[i].IsObject()) {
			return false;
		}

		if (!LoadScalingMode(scalingModesArray[i].GetObj(), scalingModes[i])) {
			return false;
		}
	}

	std::vector<ScalingMode>& settings = AppSettings::Get().ScalingModes();
	settings.insert(
		settings.end(),
		std::make_move_iterator(scalingModes.begin()),
		std::make_move_iterator(scalingModes.end())
	);

	_scalingModeAddedEvent();

	return true;
}

static bool LoadLegacyScalingMode(
	const rapidjson::GenericObject<true, rapidjson::Value>& scalingModeObj,
	ScalingMode& scalingMode
) {
	if (!JsonHelper::ReadString(scalingModeObj, "name", scalingMode.name)) {
		return false;
	}

	auto effectsNode = scalingModeObj.FindMember("effects");
	if (effectsNode == scalingModeObj.MemberEnd()) {
		return true;
	}

	if (!effectsNode->value.IsArray()) {
		return false;
	}

	auto effectsArray = effectsNode->value.GetArray();
	scalingMode.effects.reserve(effectsArray.Size());

	for (const rapidjson::Value& effectElem : effectsArray) {
		if (!effectElem.IsObject()) {
			return false;
		}

		auto elemObj = effectElem.GetObj();
		EffectOption& effect = scalingMode.effects.emplace_back();

		if (!JsonHelper::ReadString(elemObj, "effect", effect.name)) {
			return false;
		}

		if (effect.name == L"CatmullRom") {
			// CatmullRom 已删除，将它转换为 Bicubic
			effect.name = L"Bicubic";
			effect.parameters[L"paramB"] = 0.0f;
			effect.parameters[L"paramC"] = 0.5f;
		} else {
			// 效果已重新组织，所以需要遍历
			bool find = false;
			for (const EffectInfo& effectInfo : EffectsService::Get().Effects()) {
				if (EffectHelper::GetDisplayName(effectInfo.name) == effect.name) {
					effect.name = effectInfo.name;
					find = true;
					break;
				}
			}
			if (!find) {
				scalingMode.effects.pop_back();
				continue;
			}
		}

		for (const auto& prop : elemObj) {
			std::string_view name = prop.name.GetString();

			if (name == "effect" || name == "inlineParams" || name == "fp16") {
				// 忽略这些成员
				continue;
			} else if (name == "scale") {
				if (!prop.value.IsArray()) {
					return false;
				}

				const auto& scaleArray = prop.value.GetArray();
				if (scaleArray.Size() != 2 || !scaleArray[0].IsNumber() || !scaleArray[1].IsNumber()) {
					return false;
				}

				float x = scaleArray[0].GetFloat();
				float y = scaleArray[1].GetFloat();

				constexpr float DELTA = 1e-5f;

				if (x > DELTA) {
					if (y <= DELTA) {
						return false;
					}

					effect.scalingType = ::Magpie::Core::ScalingType::Normal;
					effect.scale = { x,y };
				} else if (x < -DELTA) {
					if (y >= -DELTA) {
						return false;
					}

					effect.scalingType = ::Magpie::Core::ScalingType::Fit;
					effect.scale = { -x,-y };
				} else {
					if (std::abs(y) > DELTA) {
						return false;
					}

					effect.scalingType = ::Magpie::Core::ScalingType::Fill;
				}
			} else {
				float value = 0.0f;
				if (prop.value.IsNumber()) {
					value = prop.value.GetFloat();
				} else if (prop.value.IsBool()) {
					value = (float)prop.value.GetBool();
				} else {
					return false;
				}

				effect.parameters[StrUtils::UTF8ToUTF16(name)] = value;
			}
		}
	}

	return true;
}

bool ScalingModesService::ImportLegacy(const rapidjson::Document& doc) noexcept {
	if (!doc.IsArray()) {
		return false;
	}

	auto rootArray = doc.GetArray();
	const rapidjson::SizeType size = rootArray.Size();
	if (size == 0) {
		return true;
	}

	std::vector<ScalingMode> scalingModes;
	scalingModes.resize(size);

	for (rapidjson::SizeType i = 0; i < size; ++i) {
		if (!rootArray[i].IsObject()) {
			return false;
		}

		if (!LoadLegacyScalingMode(rootArray[i].GetObj(), scalingModes[i])) {
			return false;
		}
	}

	std::vector<ScalingMode>& settings = AppSettings::Get().ScalingModes();
	settings.insert(
		settings.end(),
		std::make_move_iterator(scalingModes.begin()),
		std::make_move_iterator(scalingModes.end())
	);

	_scalingModeAddedEvent();
	
	return true;
}

}
