#include "pch.h"
#include "LocalizationService.h"
#include "AppSettings.h"
#include <winrt/Windows.System.UserProfile.h>
#include "Logger.h"

using namespace winrt;
using namespace Windows::ApplicationModel::Resources;
using namespace Windows::ApplicationModel::Resources::Core;

namespace winrt::Magpie::App {

static std::array<const wchar_t*, 2> SUPPORTED_LANGUAGES{
	L"en-US",
	L"zh-Hans"
};

// 非打包应用默认使用“Windows 显示语言”，这里查找“首选语言”
static const std::wstring& GetUserProfileLanguage() {
	static std::wstring result;
	if (!result.empty()) {
		return result;
	}

	ResourceContext resourceContext{ nullptr };
	ResourceMap resourcesSubTree{ nullptr };
	// 查找匹配的首选语言
	for (const hstring& language : UserProfile::GlobalizationPreferences::Languages()) {
		if (language == L"en-US") {
			result = L"en-US";
			return result;
		}

		if (!resourceContext) {
			resourceContext = ResourceContext();
			resourcesSubTree = ResourceManager::Current().MainResourceMap().GetSubtree(L"Resources");
		}

		resourceContext.QualifierValues().Insert(L"Language", language);
		if (resourcesSubTree.GetValue(L"_Tag", resourceContext).ValueAsString() != L"en-US") {
			// 支持此语言
			result = language;
			return result;
		}
	}

	// 没有支持的语言，回落到英语
	result = L"en-US";
	return result;
}

// 用于更改原生控件的语言，如 ToggleSwitch
static void SetProcessPreferredUILanguages(std::wstring_view language) noexcept {
	std::wstring doubleNull(language);
	doubleNull.push_back(L'\0');
	if (!::SetProcessPreferredUILanguages(MUI_LANGUAGE_NAME, doubleNull.c_str(), nullptr)) {
		Logger::Get().Win32Error("SetProcessPreferredUILanguages 失败");
	}
}

void LocalizationService::EarlyInitialize() {
	ResourceContext::SetGlobalQualifierValue(L"Language", GetUserProfileLanguage());
}

void LocalizationService::Initialize() {
	AppSettings& settings = AppSettings::Get();

	int language = settings.Language();
	if (language >= 0) {
		SetProcessPreferredUILanguages(SUPPORTED_LANGUAGES[language]);
		ResourceContext::SetGlobalQualifierValue(L"Language", SUPPORTED_LANGUAGES[language]);
	} else {
		SetProcessPreferredUILanguages(GetUserProfileLanguage());
		// EarlyInitialize 中已设置 ResourceContext::SetGlobalQualifierValue
	}
}

std::span<const wchar_t*> LocalizationService::SupportedLanguages() noexcept {
	return SUPPORTED_LANGUAGES;
}

}
