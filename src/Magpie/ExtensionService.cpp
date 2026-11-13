#include "pch.h"
#include "ExtensionService.h"
#include "AppSettings.h"
#include "ImageEncoderFactory.h"
#include "LocalizationService.h"
#include "StrHelper.h"
#include "Logger.h"
#include "AppFolderManager.h"

namespace Magpie {

using ExtEntryFn = void(MagpieHostApi*);

static std::string CurExtId;

static bool IsNullOrEmptyString(const char* str) noexcept {
	return !str || str[0] == '\0';
}

static HRESULT HostApiLog(MagpieLogSourceLocation sourceLocation, MagpieLogLevel level, const char* message) noexcept {
	if (!message || level < MagpieLogLevelInfo || level > MagpieLogLevelError) {
		return E_INVALIDARG;
	}

	if (sourceLocation.fileName == nullptr || sourceLocation.fileName[0] == '\0') {
		sourceLocation.fileName = "未知";
	}

	if (sourceLocation.funcName == nullptr || sourceLocation.funcName[0] == '\0') {
		sourceLocation.funcName = "未知";
	}

	SourceLocation sl(sourceLocation.line, sourceLocation.fileName, sourceLocation.funcName);

	if (level == MagpieLogLevelInfo) {
		Logger::Get().Info(message, sl);
	} else if (level == MagpieLogLevelWarn) {
		Logger::Get().Warn(message, sl);
	} else {
		Logger::Get().Error(message, sl);
	}

	return S_OK;
}

static const char* HotApiGetLanguage() noexcept {
	static std::string result = StrHelper::UTF16ToUTF8(LocalizationService::Get().GetLanguage());
	return result.c_str();
}

void ExtensionService::Initialize() noexcept {
	for (const ExtensionOption& extOption : AppSettings::Get().ExtensionOptions()) {
		if (!extOption.isEnabled) {
			continue;
		}

		if (!_LoadExtension(extOption.extId)) {
			Logger::Get().Error(StrHelper::Concat("加载插件 ", extOption.extId, " 失败"));
		}
	}
}

static std::wstring GetExtDllPath(std::string_view extId) noexcept {
	std::wstring extIdUtf16 = StrHelper::UTF8ToUTF16(extId);
	// 使用绝对路径以避免歧义
	return StrHelper::Concat(
		AppFolderManager::Get().GetWorkingDir().native(),
		L"\\", AppFolderManager::Get().GetExtDir(),
		L"\\", extIdUtf16, L"\\", extIdUtf16, L".dll");
}

bool ExtensionService::_LoadExtension(std::string_view extId) noexcept {
	std::wstring extDllPath = GetExtDllPath(extId);

	wil::unique_hmodule hDll(LoadLibraryEx(extDllPath.c_str(), NULL, 0));
	if (!hDll) {
		Logger::Get().Win32Error(StrHelper::Concat("加载 ", extId, ".dll 失败"));
		return false;
	}

	ExtEntryFn* extEntry = (ExtEntryFn*)GetProcAddress(hDll.get(), "MagpieExtEntry");
	if (!extEntry) {
		Logger::Get().Error(StrHelper::Concat("插件 ", extId, " 中未找到入口函数"));
		return false;
	}

	CurExtId = extId;

	MagpieHostApi hostApi = {
		.log = &HostApiLog,
		.getLanguage = &HotApiGetLanguage,
		.registerExtension = &ExtensionService::_HostApiRegisterExtension
	};
	extEntry(&hostApi);

	auto it = _extDatas.find(extId);
	if (it == _extDatas.end()) {
		Logger::Get().Error(StrHelper::Concat("插件 ", extId, " 注册失败"));
		return false;
	}

	it->second.idx = (uint32_t)_extDllHandles.size();
	// _extDllHandles 按 dll 加载顺序排序
	_extDllHandles.push_back(std::move(hDll));
	return true;
}

HRESULT ExtensionService::_HostApiRegisterExtension(const MagpieExtensionDesc* desc) {
	if (IsNullOrEmptyString(desc->displayName)) {
		return E_INVALIDARG;
	}

	if (desc->imageEncoderDescs) {
		for (uint32_t i = 0; i < desc->imageEncoderCount; ++i) {
			const MagpieImageEncoderDesc& encoderDesc = desc->imageEncoderDescs[i];

			if (IsNullOrEmptyString(encoderDesc.id) ||
				IsNullOrEmptyString(encoderDesc.displayName) ||
				IsNullOrEmptyString(encoderDesc.fileExtension)
			) {
				return E_INVALIDARG;
			}

			if (encoderDesc.optionDescs) {
				for (uint32_t j = 0; j < encoderDesc.optionCount; ++j) {
					const MagpieImageEncoderOptionDesc& optionDesc = encoderDesc.optionDescs[j];

					if (IsNullOrEmptyString(optionDesc.id) ||
						IsNullOrEmptyString(optionDesc.displayName) ||
						optionDesc.type < MagpieOptionTypeBool ||
						optionDesc.type > MagpieOptionTypeSelection
					) {
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
							return E_INVALIDARG;
						}
						break;
					}
					case MagpieOptionTypeSelection:
					{
						const MagpieSelectionOptionDesc& selectionOption = optionDesc.selectionOption;
						if (selectionOption.defaultValue >= selectionOption.itemCount) {
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

	_ExtensionData& extData = ExtensionService::Get()._extDatas[CurExtId];
	extData.displayName = desc->displayName;
	extData.author = desc->author ? desc->author : "";
	extData.homePageUrl = desc->homePageUrl ? desc->homePageUrl : "";
	extData.description = desc->description ? desc->description : "";

	if (desc->imageEncoderDescs) {
		for (uint32_t i = 0; i < desc->imageEncoderCount; ++i) {
			const MagpieImageEncoderDesc& apiEncoderDesc = desc->imageEncoderDescs[i];

			ImageEncoderMetadata metadata = {
				.displayName = apiEncoderDesc.displayName,
				.description = apiEncoderDesc.description ? apiEncoderDesc.description : "",
				.fileExtension = apiEncoderDesc.fileExtension
			};

			if (apiEncoderDesc.optionDescs) {
				metadata.optionDescs.resize(apiEncoderDesc.optionCount);
				for (uint32_t j = 0; j < apiEncoderDesc.optionCount; ++j) {
					const MagpieImageEncoderOptionDesc& apiOptionDesc = apiEncoderDesc.optionDescs[j];
					ImageEncoderOptionDesc& optionDesc = metadata.optionDescs[j];

					optionDesc.id = apiOptionDesc.id;
					optionDesc.displayName = apiOptionDesc.displayName;
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
			
			ImageEncoderFactory::Get().RegisterImageEncoder(
				StrHelper::Concat(CurExtId, "/", apiEncoderDesc.id),
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
