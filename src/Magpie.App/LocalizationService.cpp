#include "pch.h"
#include "LocalizationService.h"
#include "AppSettings.h"

using namespace winrt;
using namespace Windows::ApplicationModel::Resources;
using namespace Windows::ApplicationModel::Resources::Core;

namespace winrt::Magpie::App {

static std::array<const wchar_t*, 2> SUPPORTED_LANGUAGES{
	L"en-US",
	L"zh-Hans"
};

void LocalizationService::Initialize() {
	AppSettings& settings = AppSettings::Get();

	int language = settings.Language();
	if (language < 0) {
		// 使用系统设置，不支持的语言回落到英语
		// Magpie 已经配置为不存在的字符串回落到英语，这里显式设置 Language 是为了压制 WinUI 控件的本地化
		if (ResourceLoader::GetForCurrentView().GetString(L"_Tag") == L"en-US") {
			ResourceContext::SetGlobalQualifierValue(L"Language", L"en-US");
		}
	} else {
		ResourceContext::SetGlobalQualifierValue(L"Language", SUPPORTED_LANGUAGES[language]);
	}
}

std::span<const wchar_t*> LocalizationService::GetSupportedLanguages() noexcept {
	return SUPPORTED_LANGUAGES;
}

}
