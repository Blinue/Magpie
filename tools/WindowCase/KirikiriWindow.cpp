#include "pch.h"
#include "KirikiriWindow.h"

bool KirikiriWindow::Create(HINSTANCE hInst) noexcept {
	static const wchar_t* OWNER_NAME = L"KirikiriOwnerWindow";
	static const wchar_t* WINDOW_NAME = L"KirikiriWindow";

	WNDCLASSEXW wcex{
		.cbSize = sizeof(WNDCLASSEX),
		.style = CS_HREDRAW | CS_VREDRAW,
		.lpfnWndProc = DefWindowProc,
		.hInstance = hInst,
		.hCursor = LoadCursor(nullptr, IDC_ARROW),
		.hbrBackground = HBRUSH(COLOR_WINDOW + 1),
		.lpszClassName = OWNER_NAME
	};
	if (!RegisterClassEx(&wcex)) {
		return false;
	}

	HWND hwndOwner = CreateWindowEx(
		0,
		OWNER_NAME,
		OWNER_NAME,
		WS_OVERLAPPED,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		0,
		0,
		NULL,
		NULL,
		hInst,
		NULL
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
		hwndOwner,
		NULL,
		hInst,
		this
	);
	if (!Handle()) {
		return false;
	}

	ShowWindow(hwndOwner, SW_SHOWNORMAL);
	ShowWindow(Handle(), SW_SHOWNORMAL);
	return true;
}

LRESULT KirikiriWindow::_MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
	switch (msg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}

	return base_type::_MessageHandler(msg, wParam, lParam);
}
