#include "pch.h"
#include "AppSettings.h"
#include "LocalizationService.h"
#include <bcp47mrm.h>
#include <winrt/Windows.System.UserProfile.h>

using namespace winrt;

namespace Magpie {

// 标签必须为小写
static std::array SUPPORTED_LANGUAGES{
	L"de",
	L"en-us",
	L"es",
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
	for (const hstring& language : UserProfile::GlobalizationPreferences::Languages()) {
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

	_Language(bestLanguage);
}

void LocalizationService::Initialize() {
	AppSettings& settings = AppSettings::Get();

	int language = settings.Language();
	if (language >= 0) {
		_Language(SUPPORTED_LANGUAGES[language]);
	}
}

std::span<const wchar_t*> LocalizationService::SupportedLanguages() noexcept {
	return SUPPORTED_LANGUAGES;
}

void LocalizationService::_Language(const wchar_t* tag) {
	_language = tag;
	ResourceContext::SetGlobalQualifierValue(L"Language", tag);
}

}
