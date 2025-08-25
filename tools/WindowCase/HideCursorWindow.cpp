#include "pch.h"
#include "HideCursorWindow.h"
#include "Utils.h"

bool HideCursorWindow::Create() noexcept {
	static const wchar_t* WINDOW_NAME = L"HideCursorWindow";

	WNDCLASSEXW wcex{
		.cbSize = sizeof(WNDCLASSEX),
		.style = CS_HREDRAW | CS_VREDRAW,
		.lpfnWndProc = _WndProc,
		.hInstance = Utils::GetModuleInstanceHandle(),
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
		Utils::GetModuleInstanceHandle(),
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

LRESULT HideCursorWindow::_MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
	switch (msg) {
	case WM_CREATE:
	{
		const LRESULT ret = base_type::_MessageHandler(msg, wParam, lParam);

		_hwndBtn = CreateWindow(L"BUTTON", L"未隐藏光标", WS_CHILD | WS_VISIBLE,
			0, 0, 0, 0, Handle(), (HMENU)1, Utils::GetModuleInstanceHandle(), 0);
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
			ShowCursor(_isCursorHidden);
			SetWindowText(_hwndBtn, _isCursorHidden ? L"未隐藏光标" : L"已隐藏光标");
			_isCursorHidden = !_isCursorHidden;
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

void HideCursorWindow::_UpdateButtonPos() noexcept {
	RECT clientRect;
	GetClientRect(Handle(), &clientRect);

	const double dpiScale = _DpiScale();
	const SIZE btnSize = { std::lround(120 * dpiScale),std::lround(50 * dpiScale) };
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
