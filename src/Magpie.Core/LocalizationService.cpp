#include "pch.h"
#include "LocalizationService.h"
#include <bcp47mrm.h>
#include <winrt/Windows.System.UserProfile.h>

namespace Magpie {

// 标签必须为小写
static std::array SUPPORTED_LANGUAGES = {
	L"de",
	L"en-us",
	L"es",
	L"fi",
	L"fr",
	L"id",
	L"it",
	L"ja",
	L"ko",
	L"pl",
	L"pt-br",
	L"ru",
	L"ta",
	L"tr",
	L"uk",
	L"vi",
	L"zh-hans",
	L"zh-hant"
};

void LocalizationService::EarlyInitialize() {
	// 非打包应用默认使用“Windows 显示语言”，这里自行切换至“首选语言”
	std::wstring userLanguages;
	for (const winrt::hstring& language : winrt::UserProfile::GlobalizationPreferences::Languages()) {
		userLanguages += language;
		userLanguages += L'\0';
	}
	// 要求双空结尾
	userLanguages += L'\0';

	double bestScore = 0.0;
	// 没有支持的语言则回落到英语
	const wchar_t* bestLanguage = L"en-us";
	for (const wchar_t* language : SUPPORTED_LANGUAGES) {
		double score = 0.0;
		HRESULT hr = GetDistanceOfClosestLanguageInList(language, userLanguages.data(), 0, &score);
		if (FAILED(hr)) {
			continue;
		}

		if (score > bestScore) {
			bestScore = score;
			bestLanguage = language;

			if (score == 1.0) {
				break;
			}
		}
	}

	_SetLanguage(bestLanguage);
}

void LocalizationService::Initialize(int language) {
	if (language >= 0) {
		_SetLanguage(SUPPORTED_LANGUAGES[language]);
	}
}

std::span<const wchar_t*> LocalizationService::GetSupportedLanguages() noexcept {
	return SUPPORTED_LANGUAGES;
}

winrt::hstring LocalizationService::GetLocalizedString(std::wstring_view resName) const noexcept {
	assert(_language);

	static const wchar_t* APP_RESOURCE_MAP_ID = L"Magpie/Resources";
	// 不确定 ResourceLoader 是否线程安全，为每个线程创建独立的实例
	thread_local static winrt::ResourceLoader resourceLoader =
		winrt::ResourceLoader::GetForViewIndependentUse(APP_RESOURCE_MAP_ID);
	return resourceLoader.GetString(resName);
}

void LocalizationService::_SetLanguage(const wchar_t* tag) {
	_language = tag;
	winrt::ResourceContext::SetGlobalQualifierValue(L"Language", tag);
}


}
