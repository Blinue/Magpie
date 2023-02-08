#include "pch.h"
#include "LocalizationService.h"

using namespace winrt;
using namespace Windows::ApplicationModel::Resources::Core;
using namespace Windows::ApplicationModel::Resources;

void LocalizationService::Initialize() {
	// 不支持的语言回落到英语
	// 显式设置 Language 以压制 WinUI 控件的本地化
	if (ResourceLoader::GetForCurrentView().GetString(L"Qualifier") == L"en-US") {
		ResourceContext::SetGlobalQualifierValue(L"Language", L"en-US");
	}
}
