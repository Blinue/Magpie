#include "pch.h"
#include "PopupHostWindow.h"

static const wchar_t* WINDOW_NAME = L"PopupHostWindow";
static const wchar_t* POPUP_WINDOW_NAME = L"PopupWindow";

bool PopupHostWindow::Create(HINSTANCE hInst) noexcept {
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

	wcex.lpfnWndProc = DefWindowProc;
	wcex.lpszClassName = POPUP_WINDOW_NAME;
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

static RECT MonitorRectFromWindow(HWND hWnd) noexcept {
	HMONITOR hMon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi{ sizeof(mi) };
	GetMonitorInfo(hMon, &mi);
	return mi.rcMonitor;
}

LRESULT PopupHostWindow::_MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
	switch (msg) {
	case WM_CREATE:
	{
		const LRESULT ret = base_type::_MessageHandler(msg, wParam, lParam);

		const HMODULE hInst = GetModuleHandle(nullptr);
		_hwndBtn1 = CreateWindow(L"BUTTON", L"模态弹窗",
			WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, Handle(), (HMENU)1, hInst, 0);
		_hwndBtn2 = CreateWindow(L"BUTTON", L"模拟模态弹窗",
			WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, Handle(), (HMENU)2, hInst, 0);
		_hwndBtn3 = CreateWindow(L"BUTTON", L"所有者关系弹窗",
			WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, Handle(), (HMENU)3, hInst, 0);
		_hwndBtn4 = CreateWindow(L"BUTTON", L"普通弹窗",
			WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, Handle(), (HMENU)4, hInst, 0);
		_UpdateButtonPos();

		SendMessage(_hwndBtn1, WM_SETFONT, (WPARAM)_UIFont(), TRUE);
		SendMessage(_hwndBtn2, WM_SETFONT, (WPARAM)_UIFont(), TRUE);
		SendMessage(_hwndBtn3, WM_SETFONT, (WPARAM)_UIFont(), TRUE);
		SendMessage(_hwndBtn4, WM_SETFONT, (WPARAM)_UIFont(), TRUE);

		return ret;
	}
	case WM_SIZE:
	{
		_UpdateButtonPos();
		break;
	}
	case WM_COMMAND:
	{
		if (HIWORD(wParam) == BN_CLICKED) {
			const WORD btnId = LOWORD(wParam);
			const HMODULE hInst = GetModuleHandle(nullptr);
			const double dpiScale = _DpiScale();

			if (btnId == 1) {
				// 模态弹窗
				MessageBox(Handle(), L"", L"模态弹窗", MB_OK);
			} else if (btnId == 2) {
				// 模拟模态弹窗，弹出后将主窗口禁用

			} else if (btnId == 3) {
				// 所有者关系弹窗
				const RECT& monitorRect = MonitorRectFromWindow(Handle());
				const SIZE popupSize = { std::lround(300 * dpiScale), std::lround(200 * dpiScale) };

				CreateWindow(
					POPUP_WINDOW_NAME,
					L"所有者关系弹窗",
					(WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX) | WS_VISIBLE,
					(monitorRect.left + monitorRect.right - popupSize.cx) / 2,
					(monitorRect.top + monitorRect.bottom - popupSize.cy) / 2,
					popupSize.cx,
					popupSize.cy,
					Handle(),
					NULL,
					hInst,
					nullptr
				);
			} else {
				// 普通弹窗
				const RECT& monitorRect = MonitorRectFromWindow(Handle());
				const SIZE popupSize = { std::lround(300 * dpiScale), std::lround(200 * dpiScale) };

				CreateWindow(
					POPUP_WINDOW_NAME,
					L"普通弹窗",
					(WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX) | WS_VISIBLE,
					(monitorRect.left + monitorRect.right - popupSize.cx) / 2,
					(monitorRect.top + monitorRect.bottom - popupSize.cy) / 2,
					popupSize.cx,
					popupSize.cy,
					NULL,
					NULL,
					hInst,
					nullptr
				);
			}
		}
		break;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}

	return base_type::_MessageHandler(msg, wParam, lParam);
}

void PopupHostWindow::_UpdateButtonPos() noexcept {
	RECT clientRect;
	GetClientRect(Handle(), &clientRect);

	const double dpiScale = _DpiScale();
	SIZE btnSize = { std::lround(160 * dpiScale),std::lround(40 * dpiScale) };

	const LONG btnLeft = ((clientRect.right - clientRect.left) - btnSize.cx) / 2;
	const LONG windowCenterY = (clientRect.bottom - clientRect.top) / 2;
	const LONG spacing = 8;

	SetWindowPos(_hwndBtn1, NULL, btnLeft,
		windowCenterY - 2 * btnSize.cy - std::lround(1.5 * spacing * dpiScale),
		btnSize.cx, btnSize.cy, SWP_NOACTIVATE);
	SetWindowPos(_hwndBtn2, NULL, btnLeft,
		windowCenterY - btnSize.cy - std::lround(0.5 * spacing * dpiScale),
		btnSize.cx, btnSize.cy, SWP_NOACTIVATE);
	SetWindowPos(_hwndBtn3, NULL, btnLeft,
		windowCenterY + std::lround(0.5 * spacing * dpiScale),
		btnSize.cx, btnSize.cy, SWP_NOACTIVATE);
	SetWindowPos(_hwndBtn4, NULL, btnLeft,
		windowCenterY + btnSize.cy + std::lround(1.5 * spacing * dpiScale),
		btnSize.cx, btnSize.cy, SWP_NOACTIVATE);
}
