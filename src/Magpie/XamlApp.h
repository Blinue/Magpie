#pragma once
#include "pch.h"
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>
#include <winrt/Magpie.App.h>
#include "Win32Utils.h"


class XamlApp {
public:
	XamlApp() = default;

	XamlApp(const XamlApp&) = delete;
	XamlApp(XamlApp&&) = delete;

	static XamlApp& Get() {
		static XamlApp instance;
		return instance;
	}

	bool Initialize(HINSTANCE hInstance);

	int Run();

private:
	void _CreateMainWindow();

	void _ShowMainWindow() noexcept;

	void _Quit() noexcept;

	void _RestartAsElevated() noexcept;

	void _ShowTrayIcon() noexcept;

	void _HideTrayIcon() noexcept;

	void _OnResize();

	void _UpdateTheme();

	void _ResizeXamlDialog();
	void _RepositionXamlPopups(bool closeFlyoutPresenter);

	static LRESULT _WndProcStatic(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		return Get()._WndProc(hWnd, msg, wParam, lParam);
	}
	LRESULT _WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	static LRESULT _TrayIconWndProcStatic(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		return Get()._TrayIconWndProc(hWnd, msg, wParam, lParam);
	}
	LRESULT _TrayIconWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	Win32Utils::ScopedHandle _hMutex;

	HINSTANCE _hInst = NULL;
	// right 存储宽，bottom 存储高
	RECT _mainWndRect{};
	bool _isMainWndMaximized = false;

	NOTIFYICONDATA _nid{};

	winrt::Magpie::App::App _uwpApp{ nullptr };
	winrt::Magpie::App::MainPage _mainPage{ nullptr };
	// 用于防止内存泄露
	winrt::weak_ref<winrt::Magpie::App::MainPage> _weakMainPage;
	HWND _hwndMain = NULL;
	HWND _hwndXamlIsland = NULL;

	winrt::DesktopWindowXamlSource _xamlSource{ nullptr };
	winrt::com_ptr<IDesktopWindowXamlSourceNative2> _xamlSourceNative2;
};
