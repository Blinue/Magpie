#include "pch.h"
#include "MainWindow.h"
#include "CommonSharedConstants.h"
#include "Win32Utils.h"
#include "ThemeHelper.h"
#include "XamlApp.h"

namespace Magpie {

bool MainWindow::Create(HINSTANCE hInstance, const RECT& windowRect, bool isMaximized) noexcept {
	WNDCLASSEXW wcex{};
	wcex.cbSize = sizeof(wcex);
	wcex.lpfnWndProc = _WndProc;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(CommonSharedConstants::IDI_APP));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.lpszClassName = CommonSharedConstants::MAIN_WINDOW_CLASS_NAME;

	RegisterClassEx(&wcex);

	// Win11 22H2 中为了使用 Mica 背景需指定 WS_EX_NOREDIRECTIONBITMAP
	CreateWindowEx(
		Win32Utils::GetOSVersion().Is22H2OrNewer() ? WS_EX_NOREDIRECTIONBITMAP : 0,
		CommonSharedConstants::MAIN_WINDOW_CLASS_NAME,
		L"Magpie",
		WS_OVERLAPPEDWINDOW,
		windowRect.left, windowRect.top, windowRect.right, windowRect.bottom,
		nullptr,
		nullptr,
		hInstance,
		this
	);

	if (!_hWnd) {
		return false;
	}

	_SetContent(winrt::Magpie::App::MainPage());

	// Xaml 控件加载完成后显示主窗口
	_content.Loaded([this, isMaximized](winrt::IInspectable const&, winrt::RoutedEventArgs const&) -> winrt::IAsyncAction {
		co_await _content.Dispatcher().RunAsync(winrt::CoreDispatcherPriority::Normal, [hWnd(_hWnd), isMaximized]() {
			// 防止窗口显示时背景闪烁
			// https://stackoverflow.com/questions/69715610/how-to-initialize-the-background-color-of-win32-app-to-something-other-than-whit
			SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			ShowWindow(hWnd, isMaximized ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL);
			Win32Utils::SetForegroundWindow(hWnd);
		});
	});

	_content.ActualThemeChanged([this](winrt::FrameworkElement const&, winrt::IInspectable const&) {
		_UpdateTheme();
	});
	_UpdateTheme();

	return true;
}

void MainWindow::Show() const noexcept {
	if (IsIconic(_hWnd)) {
		ShowWindow(_hWnd, SW_RESTORE);
	}

	Win32Utils::SetForegroundWindow(_hWnd);
}

LRESULT MainWindow::_MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
	switch (msg) {
	case WM_GETMINMAXINFO:
	{
		// 设置窗口最小尺寸
		MINMAXINFO* mmi = (MINMAXINFO*)lParam;
		mmi->ptMinTrackSize = { 500,300 };
		return 0;
	}
	case WM_DESTROY:
	{
		XamlApp::Get().SaveSettings();
		break;
	}
	case CommonSharedConstants::WM_QUIT_MAGPIE:
	{
		XamlApp::Get().Quit();
		return 0;
	}
	case CommonSharedConstants::WM_RESTART_MAGPIE:
	{
		XamlApp::Get().Restart(false);
		return 0;
	}
	}
	return base_type::_MessageHandler(msg, wParam, lParam);
}

void MainWindow::_UpdateTheme() {
	const bool isDarkTheme = _content.ActualTheme() == winrt::ElementTheme::Dark;

	if (Win32Utils::GetOSVersion().Is22H2OrNewer()) {
		// 设置 Mica 背景
		DWM_SYSTEMBACKDROP_TYPE value = DWMSBT_MAINWINDOW;
		DwmSetWindowAttribute(_hWnd, DWMWA_SYSTEMBACKDROP_TYPE, &value, sizeof(value));
	} else {
		// 更改背景色以配合主题
		// 背景色在更改窗口大小时会短暂可见
		HBRUSH hbrOld = (HBRUSH)SetClassLongPtr(
			_hWnd,
			GCLP_HBRBACKGROUND,
			(INT_PTR)CreateSolidBrush(isDarkTheme ?
			CommonSharedConstants::DARK_TINT_COLOR : CommonSharedConstants::LIGHT_TINT_COLOR));
		if (hbrOld) {
			DeleteObject(hbrOld);
		}
		InvalidateRect(_hWnd, nullptr, TRUE);
	}

	ThemeHelper::SetWindowTheme(_hWnd, isDarkTheme);
}

}
