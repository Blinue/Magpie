#include "pch.h"
#include "PopupHostWindow.h"
#include "Utils.h"

bool PopupHostWindow::Create() noexcept {
	static const wchar_t* WINDOW_NAME = L"PopupHostWindow";

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

LRESULT PopupHostWindow::_Popup2WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_NCCREATE) {
		SetWindowLongPtr(hWnd, GWLP_USERDATA, LONG_PTR(((CREATESTRUCT*)lParam)->lpCreateParams));
	} else if (PopupHostWindow* that = (PopupHostWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA)) {
		switch (msg) {
		case WM_CREATE:
		{
			// 弹出后将主窗口禁用
			EnableWindow(that->Handle(), FALSE);
			return 0;
		}
		case WM_CLOSE:
		{
			EnableWindow(that->Handle(), TRUE);
			break;
		}
		}
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT PopupHostWindow::_MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
	switch (msg) {
	case WM_CREATE:
	{
		const LRESULT ret = base_type::_MessageHandler(msg, wParam, lParam);

		const HMODULE hInst = Utils::GetModuleInstanceHandle();
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
			const double dpiScale = _DpiScale();

			static const wchar_t* POPUP_WINDOW_NAME = L"PopupWindow";
			static const wchar_t* POPUP2_WINDOW_NAME = L"Popup2Window";

			if (btnId == 1) {
				// 模态弹窗
				MessageBox(Handle(), L"", L"模态弹窗", MB_OK);
			} else if (btnId == 2) {
				// 模拟模态弹窗
				static int _ = [] {
					WNDCLASSEXW wcex{
						.cbSize = sizeof(WNDCLASSEX),
						.style = CS_HREDRAW | CS_VREDRAW,
						.lpfnWndProc = _Popup2WndProc,
						.hInstance = Utils::GetModuleInstanceHandle(),
						.hCursor = LoadCursor(nullptr, IDC_ARROW),
						.hbrBackground = HBRUSH(COLOR_WINDOW + 1),
						.lpszClassName = POPUP2_WINDOW_NAME
					};
					RegisterClassEx(&wcex);
					return 0;
				}();

				const RECT& monitorRect = Utils::MonitorRectFromWindow(Handle());
				const SIZE popupSize = { std::lround(300 * dpiScale), std::lround(200 * dpiScale) };

				CreateWindow(
					POPUP2_WINDOW_NAME,
					L"模拟模态弹窗",
					(WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX) | WS_VISIBLE,
					(monitorRect.left + monitorRect.right - popupSize.cx) / 2,
					(monitorRect.top + monitorRect.bottom - popupSize.cy) / 2,
					popupSize.cx,
					popupSize.cy,
					Handle(),
					NULL,
					Utils::GetModuleInstanceHandle(),
					this
				);
			} else {
				// 所有者关系弹窗和普通弹窗
				static int _ = [] {
					WNDCLASSEXW wcex{
						.cbSize = sizeof(WNDCLASSEX),
						.style = CS_HREDRAW | CS_VREDRAW,
						.lpfnWndProc = DefWindowProc,
						.hInstance = Utils::GetModuleInstanceHandle(),
						.hCursor = LoadCursor(nullptr, IDC_ARROW),
						.hbrBackground = HBRUSH(COLOR_WINDOW + 1),
						.lpszClassName = POPUP_WINDOW_NAME
					};
					RegisterClassEx(&wcex);
					return 0;
				}();

				const RECT& monitorRect = Utils::MonitorRectFromWindow(Handle());
				const SIZE popupSize = { std::lround(300 * dpiScale), std::lround(200 * dpiScale) };

				CreateWindow(
					POPUP_WINDOW_NAME,
					btnId == 3 ? L"所有者关系弹窗" : L"普通弹窗",
					(WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX) | WS_VISIBLE,
					(monitorRect.left + monitorRect.right - popupSize.cx) / 2,
					(monitorRect.top + monitorRect.bottom - popupSize.cy) / 2,
					popupSize.cx,
					popupSize.cy,
					btnId == 3 ? Handle() : NULL,
					NULL,
					Utils::GetModuleInstanceHandle(),
					nullptr
				);
			}
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

void PopupHostWindow::_UpdateButtonPos() noexcept {
	RECT clientRect;
	GetClientRect(Handle(), &clientRect);

	const double dpiScale = _DpiScale();
	const SIZE btnSize = { std::lround(160 * dpiScale),std::lround(40 * dpiScale) };
	const LONG halfSpacing = std::lround(4 * dpiScale);

	const LONG btnLeft = ((clientRect.right - clientRect.left) - btnSize.cx) / 2;
	const LONG windowCenterY = (clientRect.bottom - clientRect.top) / 2;
	
	SetWindowPos(_hwndBtn1, NULL, btnLeft, windowCenterY - 2 * btnSize.cy - halfSpacing * 3,
		btnSize.cx, btnSize.cy, SWP_NOACTIVATE);
	SetWindowPos(_hwndBtn2, NULL, btnLeft, windowCenterY - btnSize.cy - halfSpacing,
		btnSize.cx, btnSize.cy, SWP_NOACTIVATE);
	SetWindowPos(_hwndBtn3, NULL, btnLeft, windowCenterY + halfSpacing,
		btnSize.cx, btnSize.cy, SWP_NOACTIVATE);
	SetWindowPos(_hwndBtn4, NULL, btnLeft, windowCenterY + btnSize.cy + halfSpacing * 3,
		btnSize.cx, btnSize.cy, SWP_NOACTIVATE);
}
