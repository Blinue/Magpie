#pragma once
#include <winrt/Magpie.App.h>
#include "Win32Utils.h"

struct IDesktopWindowXamlSourceNative2;

namespace Magpie {

class XamlApp {
public:
	static XamlApp& Get() noexcept {
		static XamlApp instance;
		return instance;
	}

	XamlApp(const XamlApp&) = delete;
	XamlApp(XamlApp&&) = delete;

	bool Initialize(HINSTANCE hInstance, const wchar_t* arguments);

	int Run();

	void Quit();

	void ShowMainWindow() noexcept;

private:
	XamlApp();
	~XamlApp();

	bool _CheckSingleInstance();

	void _InitializeLogger();

	void _CreateMainWindow();

	void _QuitWithoutMainWindow();

	void _Restart(bool asElevated = false, const wchar_t* arguments = nullptr) noexcept;

	void _OnResize();

	void _UpdateTheme();

	void _RepositionXamlPopups(bool closeFlyoutPresenter);

	static LRESULT _WndProcStatic(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		return Get()._WndProc(hWnd, msg, wParam, lParam);
	}
	LRESULT _WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	Win32Utils::ScopedHandle _hSingleInstanceMutex;

	HINSTANCE _hInst = NULL;
	// right 存储宽，bottom 存储高
	RECT _mainWndRect{};
	bool _isMainWndMaximized = false;

	winrt::Magpie::App::App _uwpApp{ nullptr };
	winrt::Magpie::App::MainPage _mainPage{ nullptr };
	HWND _hwndMain = NULL;
	HWND _hwndXamlIsland = NULL;

	winrt::DesktopWindowXamlSource _xamlSource{ nullptr };
	winrt::com_ptr<IDesktopWindowXamlSourceNative2> _xamlSourceNative2;
};

}
