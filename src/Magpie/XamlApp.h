#pragma once
#include "pch.h"
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>
#include <winrt/Magpie.UI.h>
#include "Win32Utils.h"


namespace Magpie {

class XamlApp {
public:
	XamlApp() = default;

	XamlApp(const XamlApp&) = delete;
	XamlApp(XamlApp&&) = delete;

	static XamlApp& Get() {
		static XamlApp instance;
		return instance;
	}

	bool Initialize(HINSTANCE hInstance, const wchar_t* arguments);

	int Run();

private:
	bool _CheckSingleInstance();

	void _InitializeLogger();

	void _CreateMainWindow();

	void _ShowMainWindow() noexcept;

	void _Quit() noexcept;

	void _RestartAsElevated(const wchar_t* arguments = nullptr) noexcept;

	void _ShowTrayIcon() noexcept;

	void _HideTrayIcon() noexcept;

	void _OnResize();

	void _UpdateTheme();

	void _ResizeContentDialog();
	void _RepositionXamlPopups(bool closeFlyoutPresenter);

	static LRESULT _WndProcStatic(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		return Get()._WndProc(hWnd, msg, wParam, lParam);
	}
	LRESULT _WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static LRESULT _TrayIconWndProcStatic(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		return Get()._TrayIconWndProc(hWnd, msg, wParam, lParam);
	}
	LRESULT _TrayIconWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	Win32Utils::ScopedHandle _hSingleInstanceMutex;

	HINSTANCE _hInst = NULL;
	// right 存储宽，bottom 存储高
	RECT _mainWndRect{};
	bool _isMainWndMaximized = false;

	NOTIFYICONDATA _nid{};

	winrt::Magpie::UI::App _uwpApp{ nullptr };
	winrt::Magpie::UI::MainPage _mainPage{ nullptr };
	HWND _hwndMain = NULL;
	HWND _hwndXamlIsland = NULL;

	winrt::DesktopWindowXamlSource _xamlSource{ nullptr };
	winrt::com_ptr<IDesktopWindowXamlSourceNative2> _xamlSourceNative2;

	bool _isTrayIconCreated = false;
};

}
