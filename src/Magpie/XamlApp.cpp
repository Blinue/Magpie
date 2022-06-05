#include "pch.h"
#include "XamlApp.h"
#include "Logger.h"
#include "StrUtils.h"
#include "Utils.h"
#include <winrt/Windows.UI.Core.h>
#include <CoreWindow.h>
#include <uxtheme.h>
#include <fmt/xchar.h>

#pragma comment(lib, "UxTheme.lib")


namespace winrt {
using namespace Windows::UI::Xaml::Hosting;
using namespace Magpie;
}


static constexpr const wchar_t* XAML_HOST_CLASS_NAME = L"Magpie_XamlHost";
static constexpr const wchar_t* MUTEX_NAME = L"{4C416227-4A30-4A2F-8F23-8701544DD7D6}";

static constexpr UINT CHECK_FORGROUND_TIMER_ID = 1;


bool XamlApp::Initialize(HINSTANCE hInstance) {
	_hMutex.reset(CreateMutex(nullptr, TRUE, MUTEX_NAME));
	if (!_hMutex || GetLastError() == ERROR_ALREADY_EXISTS) {
		// 将已存在的窗口带到前台
		// 可以唤醒旧版本，但旧版不能唤醒新版
		HWND hWnd = FindWindow(XAML_HOST_CLASS_NAME, nullptr);
		if (hWnd) {
			// 如果已有实例权限更高 ShowWindow 会失败
			ShowWindow(hWnd, SW_NORMAL);
			Utils::SetForegroundWindow(hWnd);
		} else {
			// 唤醒旧版本
			PostMessage(HWND_BROADCAST, RegisterWindowMessage(L"WM_SHOWME"), 0, 0);
		}

		return false;
	}

	winrt::init_apartment(winrt::apartment_type::single_threaded);

	// 当前目录始终是程序所在目录
	{
		wchar_t curDir[MAX_PATH] = { 0 };
		GetModuleFileName(NULL, curDir, MAX_PATH);

		for (int i = lstrlenW(curDir) - 1; i >= 0; --i) {
			if (curDir[i] == L'\\' || curDir[i] == L'/') {
				break;
			} else {
				curDir[i] = L'\0';
			}
		}

		SetCurrentDirectory(curDir);
	}

	_settings = winrt::Settings();
	if (!_settings.Initialize((uint64_t)&Logger::Get())) {
		return false;
	}

	// 注册窗口类
	{
		WNDCLASSEXW wcex{};

		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.lpfnWndProc = _WndProcStatic;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = hInstance;
		wcex.hIcon = NULL;
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.hbrBackground = NULL;
		wcex.lpszMenuName = NULL;
		wcex.lpszClassName = XAML_HOST_CLASS_NAME;
		wcex.hIconSm = NULL;

		RegisterClassEx(&wcex);
	}

	const winrt::Rect& windowRect = _settings.WindowRect();

	_hwndXamlHost = CreateWindow(
		XAML_HOST_CLASS_NAME,
		L"Magpie",
		WS_OVERLAPPEDWINDOW,
		(LONG)windowRect.X, (LONG)windowRect.Y, (LONG)windowRect.Width, (LONG)windowRect.Height,
		nullptr,
		nullptr,
		hInstance,
		nullptr
	);

	if (!_hwndXamlHost) {
		Logger::Get().Win32Error("CreateWindow 失败");
		return false;
	}

	bool isWin11 = Utils::GetOSBuild() >= 22000;
	if (isWin11) {
		// 标题栏不显示图标和标题，因为目前 DWM 存在 bug 无法在启用 Mica 时正确绘制标题
		WTA_OPTIONS option{};
		option.dwFlags = WTNCA_NODRAWCAPTION | WTNCA_NODRAWICON | WTNCA_NOSYSMENU;
		option.dwMask = WTNCA_VALIDBITS;
		SetWindowThemeAttribute(_hwndXamlHost, WTA_NONCLIENT, &option, sizeof(option));
	}

	// 初始化 UWP 应用和 XAML Islands
	_uwpApp = winrt::App();
	if (!isWin11) {
		// 防止关闭时出现 DesktopWindowXamlSource 窗口
		winrt::CoreWindow coreWindow = winrt::CoreWindow::GetForCurrentThread();
		if (coreWindow) {
			HWND hwndDWXS;
			coreWindow.as<ICoreWindowInterop>()->get_WindowHandle(&hwndDWXS);
			ShowWindow(hwndDWXS, SW_HIDE);
		}
	}

	_uwpApp.Initialize(_settings, (uint64_t)_hwndXamlHost);
	// 未显示窗口时视为位于前台，否则显示窗口的动画有小瑕疵
	_uwpApp.OnHostWndFocusChanged(true);

	_mainPage = winrt::MainPage();
	_mainPage.Loaded({ this, &XamlApp::_MainPage_Loaded });

	_xamlSource = winrt::DesktopWindowXamlSource();
	_xamlSourceNative2 = _xamlSource.as<IDesktopWindowXamlSourceNative2>();

	auto interop = _xamlSource.as<IDesktopWindowXamlSourceNative>();
	interop->AttachToWindow(_hwndXamlHost);
	interop->get_WindowHandle(&_hwndXamlIsland);
	_xamlSource.Content(_mainPage);

	// 焦点始终位于 _hwndXamlIsland 中
	_xamlSource.TakeFocusRequested([](winrt::DesktopWindowXamlSource const& sender, winrt::DesktopWindowXamlSourceTakeFocusRequestedEventArgs const& args) {
		winrt::XamlSourceFocusNavigationReason reason = args.Request().Reason();
		if (reason < winrt::XamlSourceFocusNavigationReason::Left) {
			sender.NavigateFocus(args.Request());
		}
	});

	if (isWin11) {
		// SetTimer 之前推荐先调用 SetUserObjectInformation
		BOOL value = FALSE;
		if (!SetUserObjectInformation(GetCurrentProcess(), UOI_TIMERPROC_EXCEPTION_SUPPRESSION, &value, sizeof(value))) {
			Logger::Get().Win32Error("SetUserObjectInformation 失败");
		}

		// 监听 WM_ACTIVATE 不完全可靠，因此定期检查前台窗口以确保背景绘制正确
		if (SetTimer(_hwndXamlHost, CHECK_FORGROUND_TIMER_ID, 250, nullptr) == 0) {
			Logger::Get().Win32Error("SetTimer 失败");
		}
	}

	return true;
}

int XamlApp::Run() {
	MSG msg;

	// 主消息循环
	while (GetMessage(&msg, nullptr, 0, 0)) {
		BOOL processed = FALSE;
		HRESULT hr = _xamlSourceNative2->PreTranslateMessage(&msg, &processed);
		if (SUCCEEDED(hr) && processed) {
			continue;
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	_uwpApp.OnClose();

	_xamlSourceNative2 = nullptr;
	_xamlSource.Close();
	_xamlSource = nullptr;
	_mainPage = nullptr;
	_uwpApp = nullptr;

	Logger::Get().Info("程序退出");
	Logger::Get().Flush();

	return (int)msg.wParam;
}

void XamlApp::_MainPage_Loaded(winrt::IInspectable const&, winrt::RoutedEventArgs const&) {
	// 防止窗口显示时背景闪烁
	// https://stackoverflow.com/questions/69715610/how-to-initialize-the-background-color-of-win32-app-to-something-other-than-whit
	SetWindowPos(_hwndXamlHost, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOREDRAW);

	ShowWindow(_hwndXamlHost, _settings.IsWindowMaximized() ? SW_MAXIMIZE : SW_SHOW);
	SetFocus(_hwndXamlHost);
}

void XamlApp::_OnResize() {
	RECT clientRect;
	GetClientRect(_hwndXamlHost, &clientRect);
	SetWindowPos(_hwndXamlIsland, NULL, 0, 0, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, SWP_SHOWWINDOW | SWP_NOACTIVATE);
}

LRESULT XamlApp::_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_HOTKEY:
	{
		if (message >= 0) {
			return 0;
		}

		break;
	}
	case WM_KEYDOWN:
	{
		if (wParam == VK_TAB) {
			// 处理焦点
			if (_xamlSource) {
				BYTE keyboardState[256] = {};
				if (GetKeyboardState(keyboardState)) {
					winrt::XamlSourceFocusNavigationReason reason = (keyboardState[VK_SHIFT] & 0x80) ?
						winrt::XamlSourceFocusNavigationReason::Last : winrt::XamlSourceFocusNavigationReason::First;
					_xamlSource.NavigateFocus(winrt::XamlSourceFocusNavigationRequest(reason));
				}
			}
			return 0;
		}
	}
	case WM_SIZE:
	{
		_OnResize();
		_CloseAllXamlPopups();
		return 0;
	}
	case WM_WINDOWPOSCHANGED:
	{
		if (_xamlSource) {
			WINDOWPLACEMENT wp{};
			wp.length = sizeof(wp);
			if (GetWindowPlacement(_hwndXamlHost, &wp)) {
				_settings.IsWindowMaximized(wp.showCmd == SW_MAXIMIZE);

				RECT windowRect;
				if (GetWindowRect(_hwndXamlHost, &windowRect)) {
					_settings.WindowRect(winrt::Rect{
						(float)wp.rcNormalPosition.left,
						(float)wp.rcNormalPosition.top,
						float(wp.rcNormalPosition.right - wp.rcNormalPosition.left),
						float(wp.rcNormalPosition.bottom - wp.rcNormalPosition.top)
					});
				}
			}
		}

		// 交给 DefWindowProc 处理
		break;
	}
	case WM_GETMINMAXINFO:
	{
		// 设置窗口最小尺寸
		MINMAXINFO* mmi = (MINMAXINFO*)lParam;
		mmi->ptMinTrackSize = { 500,300 };
		return 0;
	}
	case WM_ACTIVATE:
	{
		if (LOWORD(wParam) != WA_INACTIVE) {
			_uwpApp.OnHostWndFocusChanged(true);
			if (_hwndXamlIsland) {
				SetFocus(_hwndXamlIsland);
			}
		} else {
			_uwpApp.OnHostWndFocusChanged(false);
			_CloseAllXamlPopups();
		}
		
		return 0;
	}
	case WM_TIMER:
	{
		if (wParam == CHECK_FORGROUND_TIMER_ID) {
			if (!IsWindowVisible(_hwndXamlHost) || GetForegroundWindow() == _hwndXamlHost) {
				_uwpApp.OnHostWndFocusChanged(true);
			} else {
				_uwpApp.OnHostWndFocusChanged(false);
				_CloseAllXamlPopups();
			}
			return 0;
		}

		break;
	}
	case WM_MOVING:
	{
		_CloseAllXamlPopups();
		return 0;
	}
	case WM_NCLBUTTONDOWN:
	{
		_CloseAllXamlPopups();
		break;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

void XamlApp::_CloseAllXamlPopups() {
	if (!_mainPage) {
		return;
	}

	Utils::CloseAllXamlPopups(_mainPage.XamlRoot());
}
