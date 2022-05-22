#include "pch.h"
#include "XamlApp.h"
#include "Utils.h"
#include <winrt/Windows.UI.Core.h>
#include <CoreWindow.h>
#include <uxtheme.h>
#include <fmt/xchar.h>

#pragma comment(lib, "UxTheme.lib")


using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::UI::ViewManagement;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Hosting;


ATOM XamlApp::_RegisterWndClass(HINSTANCE hInstance, const wchar_t* className) {
	WNDCLASSEXW wcex{};

	// 背景色遵循系统主题以避免显示时闪烁
	UISettings uiSettings;
	auto bkgColor = uiSettings.GetColorValue(UIColorType::Background);
	HBRUSH hbrBkg = CreateSolidBrush(RGB(bkgColor.R, bkgColor.G, bkgColor.B));
	
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = _WndProcStatic;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = NULL;
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = hbrBkg;
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = className;
	wcex.hIconSm = NULL;

	return RegisterClassEx(&wcex);
}

bool XamlApp::Initialize(HINSTANCE hInstance, const wchar_t* className, const wchar_t* title) {
	init_apartment(apartment_type::single_threaded);

	_RegisterWndClass(hInstance, className);

	_hwndXamlHost = CreateWindow(className, title, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 1100, 700,
		nullptr, nullptr, hInstance, nullptr);

	if (!_hwndXamlHost) {
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

	_uwpApp = Magpie::App::App{};
	_mainPage = Magpie::App::MainPage();
	_mainPage.HostWnd((uint64_t)_hwndXamlHost);

	_xamlSource = Windows::UI::Xaml::Hosting::DesktopWindowXamlSource();
	_xamlSourceNative2 = _xamlSource.as<IDesktopWindowXamlSourceNative2>();

	if (!isWin11) {
		// 在 Win10 上可能导致任务栏出现空的 DesktopWindowXamlSource 窗口
		// 见 https://github.com/microsoft/microsoft-ui-xaml/issues/6490
		// 如果不将 ShowWindow 提前，任务栏会短暂出现两个图标
		ShowWindow(_hwndXamlHost, SW_SHOW);
	}

	auto interop = _xamlSource.as<IDesktopWindowXamlSourceNative>();
	interop->AttachToWindow(_hwndXamlHost);

	interop->get_WindowHandle(&_hwndXamlIsland);
	_xamlSource.Content(_mainPage);

	if (isWin11) {
		ShowWindow(_hwndXamlHost, SW_SHOW);
		SetFocus(_hwndXamlHost);
	} else {
		_OnResize();
	}

	// 防止关闭时出现 DesktopWindowXamlSource 窗口
	auto coreWindow = CoreWindow::GetForCurrentThread();
	if (coreWindow) {
		HWND hwndDWXS;
		coreWindow.as<ICoreWindowInterop>()->get_WindowHandle(&hwndDWXS);
		ShowWindow(hwndDWXS, SW_HIDE);
	}

	// 焦点始终位于 _hwndXamlIsland 中
	_xamlSource.TakeFocusRequested([](DesktopWindowXamlSource const& sender, DesktopWindowXamlSourceTakeFocusRequestedEventArgs const& args) {
		auto reason = args.Request().Reason();
		if (reason < XamlSourceFocusNavigationReason::Left) {
			sender.NavigateFocus(args.Request());
		}
	});

	return true;
}

int XamlApp::Run() {
	MSG msg;

	// 主消息循环
	while (GetMessage(&msg, nullptr, 0, 0)) {
		if (_xamlSource) {
			BOOL processed = FALSE;
			HRESULT hr = _xamlSourceNative2->PreTranslateMessage(&msg, &processed);
			if (SUCCEEDED(hr) && processed) {
				continue;
			}
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}

void XamlApp::_OnResize() {
	RECT clientRect;
	GetClientRect(_hwndXamlHost, &clientRect);
	SetWindowPos(_hwndXamlIsland, NULL, 0, 0, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, SWP_SHOWWINDOW | SWP_NOACTIVATE);
}

LRESULT XamlApp::_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_KEYDOWN:
	{
		if (wParam == VK_TAB) {
			// 处理焦点
			if (_xamlSource) {
				BYTE keyboardState[256] = {};
				if (GetKeyboardState(keyboardState)) {
					XamlSourceFocusNavigationReason reason = (keyboardState[VK_SHIFT] & 0x80) ?
						XamlSourceFocusNavigationReason::Last : XamlSourceFocusNavigationReason::First;
					_xamlSource.NavigateFocus(XamlSourceFocusNavigationRequest(reason));
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
	case WM_GETMINMAXINFO:
	{
		// 设置窗口最小尺寸
		MINMAXINFO* mmi = (MINMAXINFO*)lParam;
		mmi->ptMinTrackSize = { 400,300 };
		return 0;
	}
	case WM_ACTIVATE:
	{
		if (LOWORD(wParam) != WA_INACTIVE) {
			_mainPage.OnHostFocusChanged(true);
			if (_hwndXamlIsland) {
				SetFocus(_hwndXamlIsland);
			}
		} else {
			_mainPage.OnHostFocusChanged(false);
			_CloseAllXamlPopups();
		}
		
		return 0;
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
		_xamlSourceNative2 = nullptr;
		_xamlSource.Close();
		_xamlSource = nullptr;
		_mainPage = nullptr;
		_uwpApp = nullptr;
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
