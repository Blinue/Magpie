#pragma once
#include "BorderlessWindow.h"
#include "RootPage.h"
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>
#include <winrt/Windows.UI.Xaml.Hosting.h>

namespace Magpie {

class MainWindow : public BorderlessWindow<MainWindow> {
	using base_type = BorderlessWindow<MainWindow>;
	friend base_type;
	friend BaseWindow<MainWindow>;

public:
	~MainWindow() noexcept;

	bool Create() noexcept;

	void Show() const noexcept;

	void HandleMessage(const MSG& msg);

	winrt::Magpie::implementation::RootPage& RootPage() const noexcept {
		return *_rootPage;
	}

	Event<uint32_t> DpiChanged;
	Event<> Destroyed;

private:
	LRESULT _MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

	bool _ShouldDrawBackground() const noexcept;

	void _DrawBackground(HDC hdc, const RECT& bkgRect) const noexcept;

	void _CreateWindow(POINT& windowPos, SIZE& windowSize, MONITORINFO& monitorInfo) noexcept;

	void _UpdateTheme() noexcept;

	static LRESULT CALLBACK _TitleBarWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

	LRESULT _TitleBarMessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

	void _ResizeTitleBarWindow() noexcept;

	winrt::com_ptr<winrt::Magpie::implementation::RootPage> _rootPage{ nullptr };

	HWND _hwndXamlIsland = NULL;
	winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource _xamlSource{ nullptr };
	winrt::com_ptr<IDesktopWindowXamlSourceNative2> _xamlSourceNative2;

	MultithreadEvent<bool>::EventRevoker _appThemeChangedRevoker;
	wil::unique_hbrush _hbrBackground;
	wil::unique_hwnd _hwndTitleBar;
	HWND _hwndMaximizeButton = NULL;

	bool _isTrackingMouse = false;
	bool _isSmoothResizeEnabled = false;
};

}
