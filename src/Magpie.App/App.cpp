#include "pch.h"
#include "App.h"
#if __has_include("App.g.cpp")
#include "App.g.cpp"
#endif
#include "Utils.h"
#include "Logger.h"


namespace winrt::Magpie::App::implementation {

App::App() {
	__super::Initialize();
	
	AddRef();
	m_inner.as<::IUnknown>()->Release();

	// 根据操作系统版本选择图标字体
	UINT build = Utils::GetOSBuild();
	Resources().Insert(
		winrt::box_value(L"SymbolThemeFontFamily"),
		Windows::UI::Xaml::Media::FontFamily(build >= 22000 ? L"Segoe Fluent Icons" : L"Segoe MDL2 Assets")
	);
}

App::~App() {
	Close();
}

void App::OnClose() {
	_settings.Save();
}

bool App::Initialize(Magpie::App::Settings settings) {
	_settings = settings;
	return true;
}

}
