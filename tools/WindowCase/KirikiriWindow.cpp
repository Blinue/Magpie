// 模拟 TVP(KIRIKIRI) 2 引擎窗口，特征是主窗口被一个零尺寸的窗口所有。模拟了大部分
// 行为，只有最小化和还原时无动画不知道如何做到的。

#include "pch.h"
#include "KirikiriWindow.h"

bool KirikiriWindow::Create(HINSTANCE hInst) noexcept {
	static const wchar_t* OWNER_NAME = L"KirikiriOwnerWindow";
	static const wchar_t* WINDOW_NAME = L"KirikiriWindow";

	WNDCLASSEXW wcex{
		.cbSize = sizeof(WNDCLASSEX),
		.style = CS_HREDRAW | CS_VREDRAW,
		.lpfnWndProc = _OwnerWndProc,
		.hInstance = hInst,
		.hCursor = LoadCursor(nullptr, IDC_ARROW),
		.hbrBackground = HBRUSH(COLOR_WINDOW + 1),
		.lpszClassName = OWNER_NAME
	};
	if (!RegisterClassEx(&wcex)) {
		return false;
	}

	_hwndOwner = CreateWindowEx(
		0,
		OWNER_NAME,
		OWNER_NAME,
		WS_POPUP | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		0,
		0,
		NULL,
		NULL,
		hInst,
		this
	);

	wcex.lpfnWndProc = _WndProc;
	wcex.lpszClassName = WINDOW_NAME;
	if (!RegisterClassEx(&wcex)) {
		return false;
	}

	CreateWindow(
		WINDOW_NAME,
		WINDOW_NAME,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		_hwndOwner,
		NULL,
		hInst,
		this
	);
	if (!Handle()) {
		return false;
	}

	ShowWindow(_hwndOwner, SW_SHOWNORMAL);
	SetWindowPos(Handle(), NULL, 0, 0, int(500 * DpiScale()), int(400 * DpiScale()),
		SWP_NOMOVE | SWP_SHOWWINDOW);
	return true;
}

LRESULT KirikiriWindow::_MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
	switch (msg) {
	case WM_SYSCOMMAND:
	{
		if ((wParam & 0xFFF0) == SC_MINIMIZE) {
			SendMessage(_hwndOwner, msg, wParam, lParam);
			// 所有者窗口最小化后此窗口自动隐藏，无需真的最小化
			return 0;
		}
		break;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}

	return base_type::_MessageHandler(msg, wParam, lParam);
}

LRESULT KirikiriWindow::_OwnerWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_NCCREATE) {
		KirikiriWindow* that = (KirikiriWindow*)(((CREATESTRUCT*)lParam)->lpCreateParams);
		assert(that && !that->_hwndOwner);
		that->_hwndOwner = hWnd;
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)that);
	} else if (KirikiriWindow* that = (KirikiriWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA)) {
		return that->_OwnerMessageHandler(msg, wParam, lParam);
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT KirikiriWindow::_OwnerMessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
	switch (msg) {
	case WM_ACTIVATE:
	{
		if (LOWORD(wParam) != WA_INACTIVE) {
			SetForegroundWindow(Handle());
		}
		return 0;
	}
	}
	return DefWindowProc(_hwndOwner, msg, wParam, lParam);
}
