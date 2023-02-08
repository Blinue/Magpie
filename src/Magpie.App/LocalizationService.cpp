#include "pch.h"
#include "LocalizationService.h"

using namespace winrt;
using namespace Windows::ApplicationModel::Resources;
using namespace Windows::ApplicationModel::Resources::Core;

static std::array<const wchar_t*, 2> SUPPORTED_LANGUAGES {
	L"en-US",
	L"zh-Hans"
};

void LocalizationService::Initialize() {
	// 不支持的语言回落到英语
	// 显式设置 Language 以压制 WinUI 控件的本地化
	if (ResourceLoader::GetForCurrentView().GetString(L"Qualifier") == L"en-US") {
		ResourceContext::SetGlobalQualifierValue(L"Language", L"en-US");
	}
}

std::span<const wchar_t*> LocalizationService::GetSupportedLanguages() const noexcept {
	return SUPPORTED_LANGUAGES;
}
