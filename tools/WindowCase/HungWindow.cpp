#include "pch.h"
#include "HungWindow.h"

bool HungWindow::Create(HINSTANCE hInst) noexcept {
	static const wchar_t* WINDOW_NAME = L"HungWindow";

	WNDCLASSEXW wcex{
		.cbSize = sizeof(WNDCLASSEX),
		.style = CS_HREDRAW | CS_VREDRAW,
		.lpfnWndProc = _WndProc,
		.hInstance = hInst,
		.hCursor = LoadCursor(nullptr, IDC_ARROW),
		.hbrBackground = HBRUSH(COLOR_WINDOW + 1),
		.lpszClassName = WINDOW_NAME
	};
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
		NULL,
		NULL,
		hInst,
		this
	);
	if (!Handle()) {
		return false;
	}

	const double dpiScale = _DpiScale();
	SetWindowPos(Handle(), NULL, 0, 0,
		std::lround(500 * dpiScale), std::lround(400 * dpiScale),
		SWP_NOMOVE | SWP_SHOWWINDOW);
	return true;
}

LRESULT HungWindow::_MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
	switch (msg) {
	case WM_CREATE:
	{
		const LRESULT ret = base_type::_MessageHandler(msg, wParam, lParam);

		const HMODULE hInst = GetModuleHandle(nullptr);
		_hwndBtn = CreateWindow(L"BUTTON", L"无响应 10 秒",
			WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, Handle(), (HMENU)1, hInst, 0);
		_UpdateButtonPos();

		SendMessage(_hwndBtn, WM_SETFONT, (WPARAM)_UIFont(), TRUE);

		return ret;
	}
	case WM_SIZE:
	{
		_UpdateButtonPos();
		break;
	}
	case WM_COMMAND:
	{
		if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == 1) {
			Sleep(10000);
		}
		break;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}

	return base_type::_MessageHandler(msg, wParam, lParam);
}

void HungWindow::_UpdateButtonPos() noexcept {
	RECT clientRect;
	GetClientRect(Handle(), &clientRect);

	const double dpiScale = _DpiScale();
	SIZE btnSize = { std::lround(120 * dpiScale),std::lround(50 * dpiScale) };
	SetWindowPos(
		_hwndBtn,
		NULL,
		((clientRect.right - clientRect.left) - btnSize.cx) / 2,
		((clientRect.bottom - clientRect.top) - btnSize.cy) / 2,
		btnSize.cx,
		btnSize.cy,
		SWP_NOACTIVATE
	);
}
