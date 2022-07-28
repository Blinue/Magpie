#include "pch.h"
#include "TrayIconService.h"
#include "CommonSharedConstants.h"
#include "Win32Utils.h"
#include "Logger.h"
#include <winrt/Magpie.App.h>


namespace winrt::Magpie::App {

static constexpr const wchar_t* NOTIFY_ICON_WINDOW_CLASS_NAME = L"Magpie_NotifyIcon";

// {D0DCEAC7-905E-4955-B660-B9EB815C2139}
static constexpr const GUID NOTIFY_ICON_GUID =
	{ 0xd0dceac7, 0x905e, 0x4955, { 0xb6, 0x60, 0xb9, 0xeb, 0x81, 0x5c, 0x21, 0x39 } };


TrayIconService::TrayIconService() {
	_hInst = GetModuleHandle(nullptr);

	WNDCLASSEXW wcex{};
	wcex.cbSize = sizeof(wcex);
	wcex.lpfnWndProc = _WndProcStatic;
	wcex.hInstance = _hInst;
	wcex.lpszClassName = NOTIFY_ICON_WINDOW_CLASS_NAME;

	RegisterClassEx(&wcex);

	_nid.cbSize = sizeof(_nid);
	_nid.uVersion = NOTIFYICON_VERSION_4;
	_nid.uCallbackMessage = CommonSharedConstants::WM_NOTIFY_ICON;
	_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_GUID;
	_nid.guidItem = NOTIFY_ICON_GUID;
}

void TrayIconService::Show() {
	if (_nid.hWnd) {
		return;
	}

	_nid.hWnd = CreateWindow(NOTIFY_ICON_WINDOW_CLASS_NAME, nullptr, WS_OVERLAPPED, 0, 0, 0, 0, NULL, NULL, _hInst, 0);
	LoadIconMetric(_hInst, MAKEINTRESOURCE(CommonSharedConstants::IDI_APP), LIM_SMALL, &_nid.hIcon);
	wcscpy_s(_nid.szTip, std::size(_nid.szTip), L"Magpie");
	
	if (!Shell_NotifyIcon(NIM_ADD, &_nid)) {
		// 创建托盘图标失败，可能是因为已经存在
		Shell_NotifyIcon(NIM_DELETE, &_nid);
		if (!Shell_NotifyIcon(NIM_ADD, &_nid)) {
			Logger::Get().Win32Error("创建托盘图标失败");
			Hide();
			return;
		}
	}
	Shell_NotifyIcon(NIM_SETVERSION, &_nid);
}

void TrayIconService::Hide() {
	if (!_nid.hWnd) {
		return;
	}

	Shell_NotifyIcon(NIM_DELETE, &_nid);
	DestroyIcon(_nid.hIcon);
	_nid.hIcon = NULL;

	DestroyWindow(_nid.hWnd);
	_nid.hWnd = NULL;
}

LRESULT TrayIconService::_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case CommonSharedConstants::WM_NOTIFY_ICON:
	{
		UINT msg = LOWORD(lParam);
		switch (msg) {
		case WM_RBUTTONUP:
		{
			HMENU hMenu = CreatePopupMenu();
			AppendMenu(hMenu, MF_STRING, 1, L"主窗口");
			AppendMenu(hMenu, MF_STRING, 2, L"退出");

			// hWnd 必须为前台窗口才能正确展示弹出菜单
			// 即使 hWnd 是隐藏的
			SetForegroundWindow(hWnd);
			BOOL selectedMenuId = TrackPopupMenuEx(hMenu, TPM_LEFTALIGN | TPM_NONOTIFY | TPM_RETURNCMD, GET_X_LPARAM(wParam), GET_Y_LPARAM(wParam), hWnd, nullptr);

			DestroyMenu(hMenu);

			switch (selectedMenuId) {
			case 1:
			{
				Hide();

				HWND hwndHost = (HWND)Application::Current().as<App>().HwndHost();
				ShowWindow(hwndHost, SW_SHOW);
				break;
			}
			case 2:
			{
				Hide();

				HWND hwndHost = (HWND)Application::Current().as<App>().HwndHost();
				DestroyWindow(hwndHost);
				break;
			}
			default:
				break;
			}
			break;
		}
		default:
			break;
		}

		return 0;
	}
	default:
		break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

}
