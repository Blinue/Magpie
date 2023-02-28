#include "pch.h"
#include "LocalizationService.h"
#include "AppSettings.h"
#include <winrt/Windows.System.UserProfile.h>

using namespace winrt;
using namespace Windows::ApplicationModel::Resources;
using namespace Windows::ApplicationModel::Resources::Core;

namespace winrt::Magpie::App {

static std::array<const wchar_t*, 2> SUPPORTED_LANGUAGES{
	L"en-US",
	L"zh-Hans"
};

// 非打包应用默认使用“Windows 显示语言”，这里自行切换至“首选语言”
static void SetAsUserProfileLanguage() {
	ResourceContext resourceContext{ nullptr };
	ResourceMap resourcesSubTree{ nullptr };
	// 查找匹配的首选语言
	for (const hstring& language : UserProfile::GlobalizationPreferences::Languages()) {
		if (language == L"en-US") {
			ResourceContext::SetGlobalQualifierValue(L"Language", L"en-US");
			return;
		}

		if (!resourceContext) {
			resourceContext = ResourceContext();
			resourcesSubTree = ResourceManager::Current().MainResourceMap().GetSubtree(L"Resources");
		}

		resourceContext.QualifierValues().Insert(L"Language", language);
		if (resourcesSubTree.GetValue(L"_Tag", resourceContext).ValueAsString() != L"en-US") {
			// 支持此语言
			ResourceContext::SetGlobalQualifierValue(L"Language", language);
			return;
		}
	}

	// 没有支持的语言，回落到英语
	ResourceContext::SetGlobalQualifierValue(L"Language", L"en-US");
}

void LocalizationService::EarlyInitialize() {
	SetAsUserProfileLanguage();
}

void LocalizationService::Initialize() {
	AppSettings& settings = AppSettings::Get();

	int language = settings.Language();
	if (language >= 0) {
		ResourceContext::SetGlobalQualifierValue(L"Language", SUPPORTED_LANGUAGES[language]);
	}
}

std::span<const wchar_t*> LocalizationService::SupportedLanguages() noexcept {
	return SUPPORTED_LANGUAGES;
}

}
