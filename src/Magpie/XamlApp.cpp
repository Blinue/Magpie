#include "pch.h"
#include "XamlApp.h"
#include "Utils.h"
#include <winrt/Windows.UI.Core.h>
#include <CoreWindow.h>
#include <uxtheme.h>

#pragma comment(lib, "UxTheme.lib")


namespace winrt {
using namespace Windows::Foundation;
using namespace Windows::UI::ViewManagement;
}


ATOM XamlApp::_RegisterWndClass(HINSTANCE hInstance, const wchar_t* className) {
	WNDCLASSEXW wcex{};

	// 背景色遵循系统主题以避免显示时闪烁
	winrt::Windows::UI::ViewManagement::UISettings uiSettings;
	auto bkgColor = uiSettings.GetColorValue(winrt::UIColorType::Background);
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
	winrt::init_apartment(winrt::apartment_type::single_threaded);

	_RegisterWndClass(hInstance, className);

	_hwndXamlHost = CreateWindow(className, title, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 1100, 700,
		nullptr, nullptr, hInstance, nullptr);

	if (!_hwndXamlHost) {
		return false;
	}
	
	bool useMica = Utils::GetOSBuild() >= 22000;
	if (useMica) {
		// 标题栏不显示图标和标题，因为目前 DWM 存在 bug 无法在启用 Mica 时正确绘制标题
		WTA_OPTIONS option{};
		option.dwFlags = WTNCA_NODRAWCAPTION | WTNCA_NODRAWICON | WTNCA_NOSYSMENU;
		option.dwMask = WTNCA_VALIDBITS;
		SetWindowThemeAttribute(_hwndXamlHost, WTA_NONCLIENT, &option, sizeof(option));
	}

	_uwpApp = winrt::Magpie::App::App{};
	_mainPage = winrt::Magpie::App::MainPage();
	_mainPage.HostWnd((uint64_t)_hwndXamlHost);

	// 在 Win10 上可能导致任务栏出现空的 DesktopWindowXamlSource 窗口
	// 见 https://github.com/microsoft/microsoft-ui-xaml/issues/6490
	ShowWindow(_hwndXamlHost, SW_SHOW);

	_xamlSource = winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource();
	_xamlSourceNative2 = _xamlSource.as<IDesktopWindowXamlSourceNative2>();

	auto interop = _xamlSource.as<IDesktopWindowXamlSourceNative>();
	interop->AttachToWindow(_hwndXamlHost);

	interop->get_WindowHandle(&_hwndXamlIsland);
	_xamlSource.Content(_mainPage);

	_OnResize();

	// 防止关闭时出现 DesktopWindowXamlSource 窗口
	auto coreWindow = winrt::Windows::UI::Core::CoreWindow::GetForCurrentThread();
	if (coreWindow) {
		HWND hwndDWXS;
		coreWindow.as<ICoreWindowInterop>()->get_WindowHandle(&hwndDWXS);
		ShowWindow(hwndDWXS, SW_HIDE);
	}

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
	SetWindowPos(_hwndXamlIsland, NULL, 0, 0, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, SWP_SHOWWINDOW);
}

struct EnumInfo {
	HWND hwndHost = NULL;
	POINT windowMove{};
};

static BOOL CALLBACK EnumWindowsProc(
  _In_ HWND   hwnd,
  _In_ LPARAM lParam
) {
	if (!IsWindowVisible(hwnd)) {
		return TRUE;
	}

	// 过滤弹出窗口
	// 1. 父窗口为 XAML Host
	// 2. 窗口类为 Xaml_WindowedPopupClass

	EnumInfo* ei = (EnumInfo*)lParam;
	if (GetParent(hwnd) != ei->hwndHost) {
		return TRUE;
	}

	wchar_t className[256]{};
	if (!GetClassName(hwnd, className, (int)std::size(className))) {
		return TRUE;
	}

	if (className != L"Xaml_WindowedPopupClass"sv) {
		return TRUE;
	}

	RECT originRect;
	if (!GetWindowRect(hwnd, &originRect)) {
		return TRUE;
	}

	OutputDebugString((std::to_wstring(ei->windowMove.x) + L"\n").c_str());

	SetWindowPos(hwnd, NULL, originRect.left + ei->windowMove.x, originRect.top + ei->windowMove.y, 0, 0,
		SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

	return TRUE;
}

LRESULT XamlApp::_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_SIZE:
		_OnResize();
		return 0;
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
		}
		
		return 0;
	}
	case WM_MOVING:
	{
		// 使某些弹出窗口随主窗口移动（如组合框）
		RECT* targetRect = (RECT*)lParam;
		RECT curRect;
		GetWindowRect(_hwndXamlHost, &curRect);

		if (Utils::GetSizeOfRect(curRect) == Utils::GetSizeOfRect(*targetRect)) {
			EnumInfo ei{ _hwndXamlHost, POINT{targetRect->left - curRect.left, targetRect->top - curRect.top} };
			EnumWindows(EnumWindowsProc, (LPARAM)&ei);
		}

		return 0;
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
