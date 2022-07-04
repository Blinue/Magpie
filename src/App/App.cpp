#include "pch.h"
#include "App.h"
#if __has_include("App.g.cpp")
#include "App.g.cpp"
#endif
#include "Win32Utils.h"
#include "Logger.h"
#include "HotkeyService.h"
#include "AppSettings.h"


using namespace winrt;
using namespace Windows::UI::Xaml::Media;


namespace winrt::Magpie::App::implementation {

App::App() {
	__super::Initialize();
	
	AddRef();
	m_inner.as<::IUnknown>()->Release();

	// 根据操作系统版本设置样式
	ResourceDictionary resource = Resources();

	// 根据操作系统选择图标字体
	bool isWin11 = Win32Utils::GetOSBuild() >= 22000;
	resource.Insert(
		box_value(L"SymbolThemeFontFamily"),
		FontFamily(isWin11 ? L"Segoe Fluent Icons" : L"Segoe MDL2 Assets")
	);

	if (isWin11) {
		// Win11 中更改圆角大小
		resource.Insert(
			box_value(L"ControlCornerRadius"),
			box_value(CornerRadius{ 8,8,8,8 })
		);
		resource.Insert(
			box_value(L"NavigationViewContentGridCornerRadius"),
			box_value(CornerRadius{ 8,0,0,0 })
		);
	}
}

App::~App() {
	Close();
}

void App::OnClose() {
	AppSettings::Get().Save();
}

bool App::Initialize(uint64_t hwndHost, uint64_t pWndRect, uint64_t pIsWndMaximized) {
	_hwndHost = hwndHost;
	
	AppSettings& settings = AppSettings::Get();

	if (!settings.Initialize()) {
		return false;
	}

	const Rect& windowRect = settings.WindowRect();
	*(RECT*)pWndRect = {
		(LONG)std::lroundf(windowRect.X),
		(LONG)std::lroundf(windowRect.Y),
		(LONG)std::lroundf(windowRect.Width),
		(LONG)std::lroundf(windowRect.Height)
	};

	*(bool*)pIsWndMaximized = settings.IsWindowMaximized();

	return true;
}

void App::OnHostWndFocusChanged(bool isFocused) {
	if (isFocused == _isHostWndFocused) {
		return;
	}

	_isHostWndFocused = isFocused;
	_hostWndFocusChangedEvent(*this, isFocused);
}

void App::OnHotkeyPressed(HotkeyAction action) {
	HotkeyService::Get().OnHotkeyPressed(action);
}

}
