#include "pch.h"
#include "ExtensionService.h"
#include "AppSettings.h"
#include "ImageEncoderFactory.h"
#include "LocalizationService.h"
#include "StrHelper.h"
#include "Logger.h"
#include "AppFolderManager.h"
#include "ExtensionHelper.h"
#include "Win32Helper.h"
#include "JsonHelper.h"

namespace Magpie {

using ExtEntryFn = void(MagpieHostApi*, MagpieExtContext);

static bool IsNullOrEmptyString(const char* str) noexcept {
	return !str || str[0] == '\0';
}

void ExtensionService::Initialize() noexcept {
	SmallVectorImpl<ExtensionSetting>& extSettings = AppSettings::Get().ExtensionSettings();
	uint32_t extCount = (uint32_t)extSettings.size();

	for (uint32_t i = 0; i < extCount; ++i) {
		const auto& [extId, isEnabled] = extSettings[i];

		assert(ExtensionHelper::IsValidId(extId));
		assert(!_extDatas.contains(extId));

		Logger::Get().Info(StrHelper::Concat("正在加载插件 ", extId));

		std::wstring indexPath = StrHelper::Concat(
			AppFolderManager::Get().GetWorkingDir().native(),
			L"\\", AppFolderManager::Get().GetExtDir(),
			L"\\", StrHelper::UTF8ToUTF16(extId),
			L"\\magpie-ext.json"
		);
		std::string indexJson;
		if (!Win32Helper::ReadTextFile(indexPath.c_str(), indexJson) || indexJson.empty()) {
			Logger::Get().Error("读取 magpie-ext.json 失败");
			continue;
		}

		rapidjson::Document doc;
		doc.ParseInsitu(indexJson.data());

		if (doc.HasParseError()) {
			Logger::Get().Error(fmt::format("解析 json 失败\n\t错误码: {}", (int)doc.GetParseError()));
			continue;
		}

		if (!doc.IsObject()) {
			Logger::Get().Error("根元素不是对象");
			continue;
		}

		_LoadExtension(
			extId,
			const_cast<const rapidjson::Document&>(doc).GetObj(),
			i,
			isEnabled
		);
	}
}

void ExtensionService::Uninitialize() noexcept {
	if (_extDatas.empty()) {
		return;
	}

	SmallVector<std::pair<uint32_t, wil::unique_hmodule>> hDlls;
	hDlls.reserve(_extDatas.size());
	for (auto& pair : _extDatas) {
		hDlls.emplace_back(pair.second.loadNo, std::move(pair.second.hDll));
	}

	// 按 loadNo 从小到大排序，这样会倒序卸载
	std::sort(hDlls.begin(), hDlls.end(), [](const auto& l, const auto& r) {
		return l.first < r.first;
	});
}

bool ExtensionService::AddExtension(
	std::string_view extId,
	const rapidjson::GenericObject<true, rapidjson::Value>& indexRootObj
) noexcept {
	SmallVectorImpl<ExtensionSetting>& extSettings = AppSettings::Get().ExtensionSettings();

	_LoadExtension(extId, indexRootObj, (uint32_t)extSettings.size(), true);

	extSettings.push_back(ExtensionSetting{
		.extId = std::string(extId),
		.isEnabled = true
	});

	AppSettings::Get().SaveAsync();
	return true;
}

const Extension* ExtensionService::GetExtension(std::string_view extId) const noexcept {
	auto it = _extDatas.find(extId);
	if (it == _extDatas.end()) {
		return nullptr;
	}

	return &it->second;
}

bool ExtensionService::SetExtensionEnabled(std::string_view extId, bool enabled) noexcept {
	auto it = _extDatas.find(extId);
	if (it == _extDatas.end()) {
		return false;
	}

	bool& isEnabled = AppSettings::Get().ExtensionSettings()[it->second.idx].isEnabled;

	if (isEnabled != enabled) {
		_SetExtensionEnabledImpl(*it, enabled);

		isEnabled = enabled;
		AppSettings::Get().SaveAsync();
	}

	return true;
}

void ExtensionService::_SetExtensionEnabledImpl(_ExtContextData& contextData, bool enabled) noexcept {
	if (enabled) {
		if (contextData.second.hasDll) {
			_LoadExtensionDll(contextData);
		}
	} else {
		for (const std::string& id : contextData.second.imageEncoderFullIds) {
			if (!ImageEncoderFactory::Get().UnregisterImageEncoder(id)) {
				Logger::Get().Error(StrHelper::Concat("取消注册图像编码器 ", id, " 失败"));
			}
		}

		contextData.second.imageEncoderFullIds.clear();
		contextData.second.effectFullIds.clear();
		contextData.second.hDll.reset();
	}
}

void ExtensionService::_LoadExtension(
	std::string_view extId,
	const rapidjson::GenericObject<true, rapidjson::Value>& indexRootObj,
	uint32_t idx,
	bool enabled
) noexcept {
	assert(ExtensionHelper::IsValidId(extId));
	assert(!_extDatas.contains(extId));

	_ExtContextData& contextData = *_extDatas.emplace(extId, _ExtensionData{}).first;
	contextData.second.idx = idx;

	JsonHelper::ReadString(indexRootObj, "name", contextData.second.name);
	// 未提供名字时显示 ID
	if (contextData.second.name.empty()) {
		contextData.second.name = StrHelper::Concat("{", extId, "}");
	}

	JsonHelper::ReadString(indexRootObj, "description", contextData.second.description);
	JsonHelper::ReadBool(indexRootObj, "has_dll", contextData.second.hasDll);

	if (enabled) {
		_SetExtensionEnabledImpl(contextData, true);
	}
}

void ExtensionService::_LoadExtensionDll(_ExtContextData& contextData) noexcept {
	assert(contextData.second.hasDll);

	contextData.second.loadNo = _nextLoadNo++;

	std::wstring extIdUtf16 = StrHelper::UTF8ToUTF16(contextData.first);
	// 使用绝对路径以避免歧义
	std::wstring dllPath = StrHelper::Concat(
		AppFolderManager::Get().GetWorkingDir().native(),
		L"\\", AppFolderManager::Get().GetExtDir(),
		L"\\", extIdUtf16,
		L"\\", extIdUtf16, L".dll"
	);

	// LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR 可以加载同一个文件夹中的依赖 dll
	contextData.second.hDll.reset(LoadLibraryEx(dllPath.c_str(), NULL,
		LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS));
	if (!contextData.second.hDll) {
		Logger::Get().Win32Error(StrHelper::Concat("加载 ", contextData.first, ".dll 失败"));
		contextData.second.hasError = true;
		return;
	}

	ExtEntryFn* extEntry = (ExtEntryFn*)GetProcAddress(
		contextData.second.hDll.get(), STRINGIFY(MAGPIE_EXT_ENTRY));
	if (!extEntry) {
		Logger::Get().Error(StrHelper::Concat("插件 ", contextData.first, " 中未找到入口函数"));
		contextData.second.hasError = true;
		return;
	}

	MagpieHostApi hostApi = {
		.log = &ExtensionService::_HostApiLog,
		.getLanguage = &ExtensionService::_HotApiGetLanguage,
		.registerExtension = &ExtensionService::_HostApiRegisterExtension
	};
	extEntry(&hostApi, &contextData);

	if (contextData.second.hasError) {
		Logger::Get().Error(StrHelper::Concat("插件 ", contextData.first, " 注册失败"));
	}
}

HRESULT ExtensionService::_HostApiLog(
	MagpieExtContext context,
	MagpieLogSourceLocation sourceLocation,
	MagpieLogLevel level,
	const char* message
) {
	if (!context) {
		return E_INVALIDARG;
	}
	_ExtContextData* contextData = (_ExtContextData*)context;

	size_t msgLen = message ? StrHelper::StrLen(message) : 0;
	if (msgLen == 0 || level < MagpieLogLevelInfo || level > MagpieLogLevelError) {
		return E_INVALIDARG;
	}

	if (sourceLocation.fileName == nullptr || sourceLocation.fileName[0] == '\0') {
		sourceLocation.fileName = "未知";
	}

	if (sourceLocation.funcName == nullptr || sourceLocation.funcName[0] == '\0') {
		sourceLocation.funcName = "未知";
	}

	std::string logMsg = StrHelper::Concat(
		"[", contextData->first, "]", std::string_view(message, msgLen));
	SourceLocation sl(sourceLocation.line, sourceLocation.fileName, sourceLocation.funcName);

	if (level == MagpieLogLevelInfo) {
		Logger::Get().Info(logMsg, sl);
	} else if (level == MagpieLogLevelWarn) {
		Logger::Get().Warn(logMsg, sl);
	} else {
		Logger::Get().Error(logMsg, sl);
	}

	return S_OK;
}

HRESULT ExtensionService::_HotApiGetLanguage(MagpieExtContext context, const char** language) noexcept {
	if (!context || !language) {
		return E_INVALIDARG;
	}

	static std::string result = StrHelper::UTF16ToUTF8(LocalizationService::Get().GetLanguage());
	*language = result.c_str();
	return S_OK;
}

HRESULT ExtensionService::_HostApiRegisterExtension(
	MagpieExtContext context,
	const MagpieExtensionDesc* desc
) {
	if (!context) {
		return E_INVALIDARG;
	}
	auto& [extId, extData] = *(_ExtContextData*)context;

	if (desc->imageEncoderDescs) {
		for (uint32_t i = 0; i < desc->imageEncoderCount; ++i) {
			const MagpieImageEncoderDesc& encoderDesc = desc->imageEncoderDescs[i];

			if (!ExtensionHelper::IsValidId(encoderDesc.id) ||
				IsNullOrEmptyString(encoderDesc.name) ||
				IsNullOrEmptyString(encoderDesc.fileExtension)
			) {
				extData.hasError = true;
				return E_INVALIDARG;
			}

			if (encoderDesc.optionDescs) {
				for (uint32_t j = 0; j < encoderDesc.optionCount; ++j) {
					const MagpieImageEncoderOptionDesc& optionDesc = encoderDesc.optionDescs[j];

					if (!ExtensionHelper::IsValidId(optionDesc.id) ||
						IsNullOrEmptyString(optionDesc.name) ||
						optionDesc.type < MagpieOptionTypeBool ||
						optionDesc.type > MagpieOptionTypeSelection
					) {
						extData.hasError = true;
						return E_INVALIDARG;
					}

					switch (optionDesc.type) {
					case MagpieOptionTypeInt:
					{
						const MagpieIntOptionDesc& intOption = optionDesc.intOption;
						if (intOption.defaultValue < intOption.minValue ||
							intOption.defaultValue > intOption.maxValue ||
							intOption.step == 0
						) {
							extData.hasError = true;
							return E_INVALIDARG;
						}
						break;
					}
					case MagpieOptionTypeFloat:
					{
						const MagpieFloatOptionDesc& floatOption = optionDesc.floatOption;
						if (floatOption.defaultValue < floatOption.minValue ||
							floatOption.defaultValue > floatOption.maxValue ||
							floatOption.step < FLOAT_EPSILON<float>
						) {
							extData.hasError = true;
							return E_INVALIDARG;
						}
						break;
					}
					case MagpieOptionTypeSelection:
					{
						const MagpieSelectionOptionDesc& selectionOption = optionDesc.selectionOption;
						if (selectionOption.defaultValue >= selectionOption.itemCount) {
							extData.hasError = true;
							return E_INVALIDARG;
						}
						break;
					}
					default:
						break;
					}
				}
			}
		}
	}

	if (desc->imageEncoderDescs) {
		for (uint32_t i = 0; i < desc->imageEncoderCount; ++i) {
			const MagpieImageEncoderDesc& apiEncoderDesc = desc->imageEncoderDescs[i];

			ImageEncoderMetadata metadata = {
				.name = apiEncoderDesc.name,
				.description = apiEncoderDesc.description ? apiEncoderDesc.description : "",
				.fileExtension = apiEncoderDesc.fileExtension
			};

			if (apiEncoderDesc.optionDescs) {
				metadata.optionDescs.resize(apiEncoderDesc.optionCount);
				for (uint32_t j = 0; j < apiEncoderDesc.optionCount; ++j) {
					const MagpieImageEncoderOptionDesc& apiOptionDesc = apiEncoderDesc.optionDescs[j];
					ImageEncoderOptionDesc& optionDesc = metadata.optionDescs[j];

					optionDesc.id = apiOptionDesc.id;
					optionDesc.name = apiOptionDesc.name;
					optionDesc.description = apiOptionDesc.description ? apiOptionDesc.description : "";

					switch (apiOptionDesc.type) {
					case MagpieOptionTypeBool:
						optionDesc.valueDesc = BoolOptionDesc{
							.defaultValue = apiOptionDesc.boolOption.defaultValue != 0
						};
						break;
					case MagpieOptionTypeInt:
						optionDesc.valueDesc = IntOptionDesc{
							.defaultValue = apiOptionDesc.intOption.defaultValue,
							.minValue = apiOptionDesc.intOption.minValue,
							.maxValue = apiOptionDesc.intOption.maxValue,
							.step = apiOptionDesc.intOption.step,
						};
						break;
					case MagpieOptionTypeFloat:
						optionDesc.valueDesc = FloatOptionDesc{
							.defaultValue = apiOptionDesc.floatOption.defaultValue,
							.minValue = apiOptionDesc.floatOption.minValue,
							.maxValue = apiOptionDesc.floatOption.maxValue,
							.step = apiOptionDesc.floatOption.step,
						};
						break;
					case MagpieOptionTypeString:
						optionDesc.valueDesc = StringOptionDesc{
							.defaultValue = apiOptionDesc.stringOption.defaultValue
						};
						break;
					default:
					{
						assert(apiOptionDesc.type == MagpieOptionTypeSelection);
						const MagpieSelectionOptionDesc& selectionOption = apiOptionDesc.selectionOption;

						SelectionOptionDesc valueDesc = {
							.defaultValue = selectionOption.defaultValue
						};

						if (selectionOption.items) {
							valueDesc.items.resize(selectionOption.itemCount);
							for (uint32_t k = 0; k < selectionOption.itemCount; ++k) {
								valueDesc.items[k] = selectionOption.items[k];
							}
						}
						
						optionDesc.valueDesc = std::move(valueDesc);
						break;
					}
					}
				}
			}

			const std::string& imageEncoderId = extData.imageEncoderFullIds.emplace_back(
				StrHelper::Concat(extId, "/", apiEncoderDesc.id));
			
			ImageEncoderFactory::Get().RegisterImageEncoder(
				imageEncoderId,
				std::move(metadata),
				[encode(apiEncoderDesc.encode)](
					const phmap::flat_hash_map<std::string, OptionValue>& optionValues,
					wil::zstring_view fileName,
					SizeU imageSize,
					std::span<const uint8_t> pixelData,
					uint32_t rowPitch,
					winrt::AdvancedColorKind colorKind
				) {
					SmallVector<MagpieOptionValue> extOptionValues;
					extOptionValues.reserve(optionValues.size());

					for (const auto& pair : optionValues) {
						MagpieOptionValue optionValue = {
							.id = pair.first.c_str()
						};

						switch (pair.second.index()) {
						case 0:
							optionValue.boolValue = (uint8_t)std::get<0>(pair.second);
							break;
						case 1:
							optionValue.intValue = std::get<1>(pair.second);
							break;
						case 2:
							optionValue.floatValue = std::get<2>(pair.second);
							break;
						case 3:
							optionValue.stringValue = std::get<3>(pair.second).c_str();
							break;
						default:
							optionValue.selectionValue = std::get<4>(pair.second);
							break;
						}

						extOptionValues.push_back(std::move(optionValue));
					}

					MagpieColorKind apiColorKind;
					if (colorKind == winrt::AdvancedColorKind::StandardDynamicRange) {
						apiColorKind = MagpieColorKindSDR;
					} else if (colorKind == winrt::AdvancedColorKind::WideColorGamut) {
						apiColorKind = MagpieColorKindWCG;
					} else {
						apiColorKind = MagpieColorKindHDR;
					}

					return encode(
						extOptionValues.data(),
						(uint32_t)extOptionValues.size(),
						fileName.c_str(),
						imageSize.width,
						imageSize.height,
						pixelData.data(),
						pixelData.size(),
						rowPitch,
						apiColorKind
					);
				}
			);
		}
	}

	return S_OK;
}

}
