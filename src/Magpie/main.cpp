// Magpie.cpp : 定义应用程序的入口点。
//

#include "pch.h"
#include "XamlHost.h"


// 全局变量:
HINSTANCE hInst;                                // 当前实例
const wchar_t* szTitle = L"Magpie";                  // 标题栏文本
const wchar_t* szWindowClass = L"Magpie_XamlHost";            // 主窗口类名
winrt::Magpie::App::App hostApp{ nullptr };
winrt::Magpie::App::MainPage _myUserControl{ nullptr };
XamlHost xamlHost;


// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
					 _In_opt_ HINSTANCE hPrevInstance,
					 _In_ LPWSTR    lpCmdLine,
					 _In_ int       nCmdShow) {
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	winrt::init_apartment(winrt::apartment_type::single_threaded);
	hostApp = winrt::Magpie::App::App{};

	MyRegisterClass(hInstance);

	// 执行应用程序初始化:
	if (!InitInstance(hInstance, nCmdShow)) {
		return FALSE;
	}

	MSG msg;

	// 主消息循环:
	while (GetMessage(&msg, nullptr, 0, 0)) {
		if (xamlHost.PreHandleMessage(msg)) {
			continue;
		}

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

	xamlHost.Attach(hWnd, winrt::Magpie::App::MainPage());

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
	xamlHost.HandleMessage(hWnd, message, wParam, lParam);

	switch (message)
	{
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
