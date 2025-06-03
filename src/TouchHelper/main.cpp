#include "pch.h"
#include <magnification.h>
#include "../Magpie.Core/include/CommonSharedConstants.h"

static UINT WM_MAGPIE_SCALINGCHANGED;
// 用于与主程序交互。wParam 的值:
// 0: Magpie 通知 TouchHelper 退出
// 1: TouchHelper 向缩放窗口报告结果，lParam 为 0 表示成功，否则为错误代码
static UINT WM_MAGPIE_TOUCHHELPER;
static HWND hwndScaling = NULL;

static void DisableInputTransform() noexcept {
	DWORD errorCode = 0;
	RECT ununsed{};
	if (!MagSetInputTransform(FALSE, &ununsed, &ununsed)) {
		errorCode = GetLastError();
	}

	if (hwndScaling) {
		// 报告结果
		PostMessage(hwndScaling, WM_MAGPIE_TOUCHHELPER, 1, errorCode);
	}
}

static void UpdateInputTransform() noexcept {
	assert(hwndScaling);

	RECT srcTouchRect{
		.left = (LONG)(INT_PTR)GetProp(hwndScaling, L"Magpie.SrcTouchLeft"),
		.top = (LONG)(INT_PTR)GetProp(hwndScaling, L"Magpie.SrcTouchTop"),
		.right = (LONG)(INT_PTR)GetProp(hwndScaling, L"Magpie.SrcTouchRight"),
		.bottom = (LONG)(INT_PTR)GetProp(hwndScaling, L"Magpie.SrcTouchBottom")
	};

	RECT destTouchRect{
		.left = (LONG)(INT_PTR)GetProp(hwndScaling, L"Magpie.DestTouchLeft"),
		.top = (LONG)(INT_PTR)GetProp(hwndScaling, L"Magpie.DestTouchTop"),
		.right = (LONG)(INT_PTR)GetProp(hwndScaling, L"Magpie.DestTouchRight"),
		.bottom = (LONG)(INT_PTR)GetProp(hwndScaling, L"Magpie.DestTouchBottom")
	};

	DWORD errorCode = 0;
	if (!MagSetInputTransform(TRUE, &srcTouchRect, &destTouchRect)) {
		errorCode = GetLastError();
	}

	// 报告结果
	PostMessage(hwndScaling, WM_MAGPIE_TOUCHHELPER, 1, errorCode);
}

static LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_MAGPIE_SCALINGCHANGED) {
		if (wParam == 0) {
			// 缩放结束
			if (hwndScaling) {
				hwndScaling = NULL;
				DisableInputTransform();
			}
		} else if (wParam == 1) {
			// 缩放开始
			hwndScaling = (HWND)lParam;
			UpdateInputTransform();
		} else if (wParam == 2) {
			// 缩放窗口位置或大小改变
			if (hwndScaling) {
				UpdateInputTransform();
			}
		} else if (wParam == 3) {
			// 用户开始调整缩放窗口大小或移动缩放窗口，临时禁用触控变换
			if (hwndScaling) {
				DisableInputTransform();
			}
		}

		return 0;
	} else if (msg == WM_MAGPIE_TOUCHHELPER) {
		if (wParam == 0) {
			// 退出
			DestroyWindow(hWnd);
		}

		return 0;
	}

	switch (msg) {
	case WM_CREATE:
	{
		WM_MAGPIE_SCALINGCHANGED =
			RegisterWindowMessage(CommonSharedConstants::WM_MAGPIE_SCALINGCHANGED);
		WM_MAGPIE_TOUCHHELPER =
			RegisterWindowMessage(CommonSharedConstants::WM_MAGPIE_TOUCHHELPER);

		// 防止消息被 UIPI 过滤
		ChangeWindowMessageFilterEx(hWnd, WM_MAGPIE_SCALINGCHANGED, MSGFLT_ADD, nullptr);
		ChangeWindowMessageFilterEx(hWnd, WM_MAGPIE_TOUCHHELPER, MSGFLT_ADD, nullptr);
		break;
	}
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		break;
	}
	default:
		break;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

static wil::unique_mutex_nothrow CheckSingleInstance() noexcept {
	wil::unique_mutex_nothrow hSingleInstanceMutex;

	bool alreadyExists = false;
	if (!hSingleInstanceMutex.try_create(
		CommonSharedConstants::TOUCH_HELPER_SINGLE_INSTANCE_MUTEX_NAME,
		CREATE_MUTEX_INITIAL_OWNER,
		MUTEX_ALL_ACCESS,
		nullptr,
		&alreadyExists
	) || alreadyExists) {
		hSingleInstanceMutex.reset();
	}

	return hSingleInstanceMutex;
}

// 退出前还原触控输入变换
static void CleanBeforeExit() noexcept {
	DisableInputTransform();
	MagUninitialize();
}

int APIENTRY wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE /*hPrevInstance*/,
	_In_ LPWSTR /*lpCmdLine*/,
	_In_ int /*nCmdShow*/
) {
	// 确保单例
	wil::unique_mutex_nothrow hSingleInstanceMutex = CheckSingleInstance();
	if (!hSingleInstanceMutex) {
		return 0;
	}
	auto se = hSingleInstanceMutex.ReleaseMutex_scope_exit();

	wil::unique_mutex_nothrow hMagpieMutex;
	if (!hMagpieMutex.try_create(CommonSharedConstants::SINGLE_INSTANCE_MUTEX_NAME)) {
		return 1;
	}

	if (wil::handle_wait(hMagpieMutex.get(), 0)) {
		// Magpie 未启动
		hMagpieMutex.ReleaseMutex();
		return 0;
	}
	
	if (!MagInitialize()) {
		return 1;
	}

	// 创建一个隐藏窗口用于接收广播消息
	{
		WNDCLASSEXW wcex{
			.cbSize = sizeof(wcex),
			.lpfnWndProc = WndProc,
			.hInstance = hInstance,
			.lpszClassName = CommonSharedConstants::TOUCH_HELPER_WINDOW_CLASS_NAME
		};
		RegisterClassEx(&wcex);
	}

	wil::unique_hwnd hWnd(CreateWindow(
		CommonSharedConstants::TOUCH_HELPER_WINDOW_CLASS_NAME,
		nullptr,
		WS_OVERLAPPEDWINDOW | WS_POPUP,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		hInstance,
		0
	));
	if (!hWnd) {
		return 1;
	}

	{
		// 检查 Magpie 是否正在缩放，注意如果缩放窗口尚未显示视为没有缩放，
		// 此时缩放窗口正在初始化，会在完成后广播 WM_MAGPIE_SCALINGCHANGED 消息
		HWND hwndFound = FindWindow(CommonSharedConstants::SCALING_WINDOW_CLASS_NAME, nullptr);
		if (hwndFound && IsWindowVisible(hwndFound)) {
			hwndScaling = hwndFound;
			UpdateInputTransform();
		}
	}

	MSG msg;
	while (true) {
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				CleanBeforeExit();
				return (int)msg.wParam;
			}

			DispatchMessage(&msg);
		}

		// 等待新消息或 Magpie 退出
		if (MsgWaitForMultipleObjectsEx(1, hMagpieMutex.addressof(),
			INFINITE, QS_ALLINPUT, MWMO_INPUTAVAILABLE) == WAIT_OBJECT_0) {
			// Magpie 已退出
			CleanBeforeExit();
			hMagpieMutex.ReleaseMutex();
			return 0;
		}
	}
}
