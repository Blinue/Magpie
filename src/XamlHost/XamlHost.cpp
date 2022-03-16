// Magpie.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.system.h>
#include <winrt/windows.ui.xaml.hosting.h>
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>
#include <winrt/windows.ui.xaml.controls.h>
#include <winrt/Windows.ui.xaml.media.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Magpie.h>
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")

using namespace winrt;
using namespace Windows::UI;
using namespace Windows::UI::Composition;
using namespace Windows::UI::Xaml::Hosting;
using namespace Windows::Foundation::Numerics;
using namespace Windows::UI::Xaml::Controls;

// 全局变量:
HINSTANCE hInst;                                // 当前实例
const wchar_t* szTitle = L"Magpie";                  // 标题栏文本
const wchar_t* szWindowClass = L"Magpie_XamlHost";            // 主窗口类名
winrt::Magpie::App hostApp{ nullptr };
winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource _desktopWindowXamlSource{ nullptr };
winrt::Magpie::MainPage _myUserControl{ nullptr };

// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void AdjustLayout(HWND);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
					 _In_opt_ HINSTANCE hPrevInstance,
					 _In_ LPWSTR    lpCmdLine,
					 _In_ int       nCmdShow) {
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	winrt::init_apartment(winrt::apartment_type::single_threaded);
	hostApp = winrt::Magpie::App{};
	_desktopWindowXamlSource = winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource{};

	MyRegisterClass(hInstance);

	// 执行应用程序初始化:
	if (!InitInstance(hInstance, nCmdShow)) {
		return FALSE;
	}

	MSG msg;

	// 主消息循环:
	while (GetMessage(&msg, nullptr, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}


//
//  函数: MyRegisterClass()
//
//  目标: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex{};

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style          = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc    = WndProc;
	wcex.cbClsExtra     = 0;
	wcex.cbWndExtra     = 0;
	wcex.hInstance      = hInstance;
	wcex.hIcon = NULL;
	wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName  = szWindowClass;
	wcex.hIconSm = NULL;

	return RegisterClassEx(&wcex);
}

RTL_OSVERSIONINFOW GetOSVersion() {
	HMODULE hNtDll = GetModuleHandle(L"ntdll.dll");
	if (!hNtDll) {
		return {};
	}

	auto rtlGetVersion = (LONG(WINAPI*)(PRTL_OSVERSIONINFOW))GetProcAddress(hNtDll, "RtlGetVersion");
	if (rtlGetVersion == nullptr) {
		return {};
	}

	RTL_OSVERSIONINFOW version{};
	version.dwOSVersionInfoSize = sizeof(version);
	rtlGetVersion(&version);

	return version;
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目标: 保存实例句柄并创建主窗口
//
//   注释:
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
	hInst = hInstance; // Store instance handle in our global variable

	auto osVersion = GetOSVersion();
	bool isWin11 = osVersion.dwMajorVersion == 10 && osVersion.dwMinorVersion == 0 && osVersion.dwBuildNumber >= 22000;

	HWND hWnd = CreateWindowEx(isWin11 ? WS_EX_NOREDIRECTIONBITMAP | WS_EX_DLGMODALFRAME : 0, szWindowClass, isWin11 ? L"" : szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 1000, 700, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd) {
		return FALSE;
	}

	if (isWin11) {
		constexpr const DWORD DWMWA_MICA_EFFECT = 1029;

		BOOL value = TRUE;
		DwmSetWindowAttribute(hWnd, DWMWA_MICA_EFFECT, &value, sizeof(value));
	}

	// Begin XAML Islands walkthrough code.
	if (_desktopWindowXamlSource != nullptr) {
		auto interop = _desktopWindowXamlSource.as<IDesktopWindowXamlSourceNative>();
		check_hresult(interop->AttachToWindow(hWnd));
		HWND hWndXamlIsland = nullptr;
		interop->get_WindowHandle(&hWndXamlIsland);
		_myUserControl = winrt::Magpie::MainPage();
		_desktopWindowXamlSource.Content(_myUserControl);
	}
	// End XAML Islands walkthrough code.

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目标: 处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			// TODO: 在此处添加使用 hdc 的任何绘图代码...
			EndPaint(hWnd, &ps);
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		if (_desktopWindowXamlSource != nullptr) {
			_desktopWindowXamlSource.Close();
			_desktopWindowXamlSource = nullptr;
		}
		break;
	case WM_SIZE:
		AdjustLayout(hWnd);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void AdjustLayout(HWND hWnd) {
	if (_desktopWindowXamlSource != nullptr) {
		auto interop = _desktopWindowXamlSource.as<IDesktopWindowXamlSourceNative>();
		HWND xamlHostHwnd = NULL;
		check_hresult(interop->get_WindowHandle(&xamlHostHwnd));
		RECT windowRect;
		::GetClientRect(hWnd, &windowRect);
		::SetWindowPos(xamlHostHwnd, NULL, 0, 0, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, SWP_SHOWWINDOW);
	}
}

