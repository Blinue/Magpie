#include "pch.h"
#include "App.h"
#if __has_include("App.g.cpp")
#include "App.g.cpp"
#endif
#include "Win32Utils.h"
#include "Logger.h"


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
	_settings.Save();
}

bool App::Initialize(Magpie::App::Settings const& settings, uint64_t hwndHost) {
	_hwndHost = hwndHost;
	_settings = settings;

	_magService = Magpie::App::MagService(settings, _magRuntime);

	// HotkeyManager 中的回调总是最先调用
	_hotkeyManager = Magpie::App::HotkeyManager(settings, hwndHost);
	_hotkeyPressedRevoker = _hotkeyManager.HotkeyPressed(auto_revoke, { this, &App::_HotkeyManger_HotkeyPressed });

	return true;
}

event_token App::HostWndFocusChanged(EventHandler<bool> const& handler) {
	return _hostWndFocusChangedEvent.add(handler);
}

void App::HostWndFocusChanged(event_token const& token) noexcept {
	_hostWndFocusChangedEvent.remove(token);
}

void App::OnHostWndFocusChanged(bool isFocused) {
	if (isFocused == _isHostWndFocused) {
		return;
	}

	_isHostWndFocused = isFocused;
	_hostWndFocusChangedEvent(*this, isFocused);
}

void App::_HotkeyManger_HotkeyPressed(IInspectable const&, HotkeyAction action) {
	switch (action) {
	case HotkeyAction::Scale:
	{
		if (_magRuntime.IsRunning()) {
			_magRuntime.Stop();
			return;
		}

		HWND hwndFore = GetForegroundWindow();
		if (hwndFore) {
			_magRuntime.Run((uint64_t)hwndFore, _magSettings);
		}
		break;
	}
	case HotkeyAction::Overlay:
	{
		if (_magRuntime.IsRunning()) {
			_magRuntime.ToggleOverlay();
			return;
		}
		break;
	}
	default:
		break;
	}
}

}
