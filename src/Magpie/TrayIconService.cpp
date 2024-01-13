#include "pch.h"
#include "TrayIconService.h"
#include "CommonSharedConstants.h"
#include "Logger.h"
#include "resource.h"
#include "XamlApp.h"

namespace Magpie {

// 当任务栏被创建时会广播此消息。用于在资源管理器被重新启动时重新创建托盘图标
// https://learn.microsoft.com/en-us/windows/win32/shell/taskbar#taskbar-creation-notification
const UINT TrayIconService::_WM_TASKBARCREATED = RegisterWindowMessage(L"TaskbarCreated");

void TrayIconService::Initialize() noexcept {
	_nid.cbSize = sizeof(_nid);
	_nid.uVersion = 0;	// 不使用 NOTIFYICON_VERSION_4
	_nid.uCallbackMessage = CommonSharedConstants::WM_NOTIFY_ICON;
	_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	_nid.uID = 0;
}

void TrayIconService::Uninitialize() noexcept {
	IsShow(false);

	if (_nid.hWnd) {
		DestroyWindow(_nid.hWnd);
	}
	if (_nid.hIcon) {
		DestroyIcon(_nid.hIcon);
	}
}

void TrayIconService::IsShow(bool value) noexcept {
	_shouldShow = value;

	if (value) {
		if (!_nid.hWnd) {
			// 创建一个隐藏窗口用于接收托盘图标消息
			HINSTANCE hInst = GetModuleHandle(nullptr);
			{
				WNDCLASSEXW wcex{};
				wcex.cbSize = sizeof(wcex);
				wcex.hInstance = hInst;
				wcex.lpfnWndProc = _TrayIconWndProcStatic;
				wcex.lpszClassName = CommonSharedConstants::NOTIFY_ICON_WINDOW_CLASS_NAME;

				RegisterClassEx(&wcex);
			}

			_nid.hWnd = CreateWindow(
				CommonSharedConstants::NOTIFY_ICON_WINDOW_CLASS_NAME,
				nullptr,
				WS_OVERLAPPEDWINDOW | WS_POPUP,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				NULL,
				NULL,
				hInst,
				0
			);

			LoadIconMetric(hInst, MAKEINTRESOURCE(IDI_APP), LIM_SMALL, &_nid.hIcon);
			wcscpy_s(_nid.szTip, std::size(_nid.szTip), L"Magpie");
		}

		if (!Shell_NotifyIcon(NIM_ADD, &_nid)) {
			// 创建托盘图标失败，可能是因为已经存在
			Shell_NotifyIcon(NIM_DELETE, &_nid);
			if (!Shell_NotifyIcon(NIM_ADD, &_nid)) {
				Logger::Get().Win32Error("创建托盘图标失败");
				_isShow = false;
				return;
			}
		}

		_isShow = true;
	} else {
		if (_isShow) {
			Shell_NotifyIcon(NIM_DELETE, &_nid);
			_shouldShow = false;
			_isShow = false;
		}
	}
}

LRESULT TrayIconService::_TrayIconWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case CommonSharedConstants::WM_NOTIFY_ICON:
	{
		switch (lParam) {
		case WM_LBUTTONDBLCLK:
		{
			XamlApp::Get().ShowMainWindow();
			break;
		}
		case WM_RBUTTONUP:
		{
			winrt::ResourceLoader resourceLoader =
				winrt::ResourceLoader::GetForCurrentView(CommonSharedConstants::APP_RESOURCE_MAP_ID);
			winrt::hstring mainWindowText = resourceLoader.GetString(L"TrayIcon_MainWindow");
			winrt::hstring exitText = resourceLoader.GetString(L"TrayIcon_Exit");

			HMENU hMenu = CreatePopupMenu();
			AppendMenu(hMenu, MF_STRING, 1, mainWindowText.c_str());
			AppendMenu(hMenu, MF_STRING, 2, exitText.c_str());

			// hWnd 必须为前台窗口才能正确展示弹出菜单
			// 即使 hWnd 是隐藏的
			SetForegroundWindow(hWnd);

			POINT cursorPos;
			GetCursorPos(&cursorPos);
			BOOL selectedMenuId = TrackPopupMenuEx(hMenu, TPM_LEFTALIGN | TPM_NONOTIFY | TPM_RETURNCMD, cursorPos.x, cursorPos.y, hWnd, nullptr);

			DestroyMenu(hMenu);

			switch (selectedMenuId) {
			case 1:
			{
				XamlApp::Get().ShowMainWindow();
				break;
			}
			case 2:
			{
				XamlApp::Get().Quit();
				break;
			}
			}
			break;
		}
		}

		return 0;
	}
	case WM_WINDOWPOSCHANGING:
	{
		// 如果 Magpie 启动时任务栏尚未被创建，Shell_NotifyIcon 会失败，因此无法收到 WM_TASKBARCREATED 消息。
		// 监听 WM_WINDOWPOSCHANGING 以在资源管理器启动时获得通知
		// hack 来自 https://github.com/microsoft/PowerToys/pull/789
		if (!_isShow && _shouldShow) {
			IsShow(true);
		}
		break;
	}
	default:
	{
		if (message == _WM_TASKBARCREATED) {
			if (_shouldShow) {
				// 重新创建任务栏图标
				IsShow(true);
			}
		}
	}
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

}
