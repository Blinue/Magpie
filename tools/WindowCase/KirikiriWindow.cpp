// 模拟 TVP(KIRIKIRI) 2 引擎窗口，特征是主窗口被一个零尺寸的窗口所有。模拟了大部分
// 行为，只有最小化和还原时无动画不知道如何做到的。

#include "pch.h"
#include "KirikiriWindow.h"
#include "Utils.h"

static const wchar_t* WINDOW_NAME = L"KirikiriWindow";

bool KirikiriWindow::Create() noexcept {
	static const wchar_t* OWNER_NAME = L"KirikiriOwnerWindow";
	
	WNDCLASSEXW wcex{
		.cbSize = sizeof(WNDCLASSEX),
		.style = CS_HREDRAW | CS_VREDRAW,
		.lpfnWndProc = _OwnerWndProc,
		.hInstance = Utils::GetModuleInstanceHandle(),
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
		Utils::GetModuleInstanceHandle(),
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
		Utils::GetModuleInstanceHandle(),
		this
	);
	if (!Handle()) {
		return false;
	}

	ShowWindow(_hwndOwner, SW_SHOWNORMAL);

	const double dpiScale = _DpiScale();
	SetWindowPos(Handle(), NULL, 0, 0,
		std::lround(500 * dpiScale), std::lround(400 * dpiScale),
		SWP_NOMOVE | SWP_SHOWWINDOW);

	return true;
}

LRESULT KirikiriWindow::_MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
	if (_isPopup) {
		return _PopupMessageHandler(msg, wParam, lParam);
	}

	switch (msg) {
	case WM_CREATE:
	{
		const LRESULT ret = base_type::_MessageHandler(msg, wParam, lParam);

		const HMODULE hInst = Utils::GetModuleInstanceHandle();
		_hwndBtn1 = CreateWindow(L"BUTTON", L"同类名所有者关系弹窗",
			WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, Handle(), (HMENU)1, hInst, 0);
		_hwndBtn2 = CreateWindow(L"BUTTON", L"同类名模拟模态弹窗",
			WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, Handle(), (HMENU)2, hInst, 0);
		_UpdateButtonPos();

		SendMessage(_hwndBtn1, WM_SETFONT, (WPARAM)_UIFont(), TRUE);
		SendMessage(_hwndBtn2, WM_SETFONT, (WPARAM)_UIFont(), TRUE);

		return ret;
	}
	case WM_SIZE :
	{
		_UpdateButtonPos();
		break;
	}
	case WM_COMMAND:
	{
		if (HIWORD(wParam) == BN_CLICKED) {
			const bool isOwnedPopup = LOWORD(wParam) == 1;

			std::unique_ptr<KirikiriWindow>& curPopup = isOwnedPopup ? _popup1 : _popup2;

			if (!curPopup) {
				curPopup = std::make_unique<KirikiriWindow>();
				curPopup->_hwndMain = Handle();
				curPopup->_isPopup = true;
				curPopup->_isOwnedPopup = isOwnedPopup;
			}

			if (curPopup->Handle()) {
				SetWindowPos(curPopup->Handle(), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			} else {
				const double dpiScale = _DpiScale();
				const RECT& monitorRect = Utils::MonitorRectFromWindow(Handle());
				const SIZE popupSize = { std::lround(300 * dpiScale), std::lround(200 * dpiScale) };

				CreateWindow(
					WINDOW_NAME,
					isOwnedPopup ? L"所有者关系弹窗" : L"模拟模态弹窗",
					(WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX) | WS_VISIBLE,
					(monitorRect.left + monitorRect.right - popupSize.cx) / 2,
					(monitorRect.top + monitorRect.bottom - popupSize.cy) / 2,
					popupSize.cx,
					popupSize.cy,
					isOwnedPopup ? Handle() : NULL,
					NULL,
					Utils::GetModuleInstanceHandle(),
					curPopup.get()
				);
			}
			
			return 0;
		}
		break;
	}
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
		switch (msg) {
		case WM_ACTIVATE:
		{
			if (LOWORD(wParam) != WA_INACTIVE) {
				SetForegroundWindow(that->Handle());
			}
			return 0;
		}
		}
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT KirikiriWindow::_PopupMessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
	if (_isOwnedPopup) {
		return base_type::_MessageHandler(msg, wParam, lParam);
	}

	switch (msg) {
	case WM_CREATE:
	{
		// 弹出后将主窗口禁用
		EnableWindow(_hwndMain, FALSE);
		return 0;
	}
	case WM_CLOSE:
	{
		EnableWindow(_hwndMain, TRUE);
		break;
	}
	}

	return base_type::_MessageHandler(msg, wParam, lParam);
}

void KirikiriWindow::_UpdateButtonPos() noexcept {
	RECT clientRect;
	GetClientRect(Handle(), &clientRect);

	const double dpiScale = _DpiScale();
	const SIZE btnSize = { std::lround(170 * dpiScale),std::lround(50 * dpiScale) };
	const LONG halfSpacing = std::lround(4 * dpiScale);

	const LONG btnLeft = ((clientRect.right - clientRect.left) - btnSize.cx) / 2;
	const LONG btn1Top = (clientRect.bottom - clientRect.top) / 2
		- btnSize.cy - halfSpacing;

	SetWindowPos(_hwndBtn1, NULL, btnLeft, btn1Top, btnSize.cx, btnSize.cy, SWP_NOACTIVATE);
	SetWindowPos(_hwndBtn2, NULL, btnLeft, btn1Top + btnSize.cy + halfSpacing * 2,
		btnSize.cx, btnSize.cy, SWP_NOACTIVATE);
}
