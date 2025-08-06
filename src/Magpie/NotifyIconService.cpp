#include "pch.h"
#include "App.h"
#include "CommonSharedConstants.h"
#include "Logger.h"
#include "NotifyIconService.h"
#include "resource.h"
#include "ScalingService.h"
#include <CommCtrl.h>

using namespace winrt::Magpie::implementation;
using namespace winrt;

namespace Magpie {

// 当任务栏被创建时会广播此消息。用于在资源管理器被重新启动时重新创建托盘图标
// https://learn.microsoft.com/en-us/windows/win32/shell/taskbar#taskbar-creation-notification
static UINT WM_TASKBARCREATED = 0;

void NotifyIconService::Initialize() noexcept {
	WM_TASKBARCREATED = RegisterWindowMessage(L"TaskbarCreated");

	_nid.cbSize = sizeof(_nid);
	_nid.uVersion = 0;	// 不使用 NOTIFYICON_VERSION_4
	_nid.uCallbackMessage = CommonSharedConstants::WM_NOTIFY_ICON;
	_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	_nid.uID = 0;
}

void NotifyIconService::Uninitialize() noexcept {
	if (!WM_TASKBARCREATED) {
		return;
	}

	IsShow(false);

	if (_nid.hWnd) {
		DestroyWindow(_nid.hWnd);
	}
	if (_nid.hIcon) {
		DestroyIcon(_nid.hIcon);
	}
}

void NotifyIconService::IsShow(bool value) noexcept {
	_shouldShow = value;

	if (value) {
		if (!_nid.hWnd) {
			// 创建一个隐藏窗口用于接收托盘图标消息
			HINSTANCE hInst = wil::GetModuleInstanceHandle();
			{
				WNDCLASSEXW wcex{
					.cbSize = sizeof(wcex),
					.lpfnWndProc = _NotifyIconWndProcStatic,
					.hInstance = hInst,
					.lpszClassName = CommonSharedConstants::NOTIFY_ICON_WINDOW_CLASS_NAME
				};
				RegisterClassEx(&wcex);
			}

			_nid.hWnd = CreateWindow(
				CommonSharedConstants::NOTIFY_ICON_WINDOW_CLASS_NAME,
				nullptr,
				WS_OVERLAPPEDWINDOW,
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

LRESULT NotifyIconService::_NotifyIconWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case CommonSharedConstants::WM_NOTIFY_ICON:
	{
		switch (lParam) {
		case WM_LBUTTONUP:
		{
			App::Get().ShowMainWindow();
			break;
		}
		case WM_RBUTTONUP:
		{
			wil::unique_hmenu hMenu(CreatePopupMenu());

			ResourceLoader resourceLoader =
				ResourceLoader::GetForCurrentView(CommonSharedConstants::APP_RESOURCE_MAP_ID);

			hstring mainWindowText = resourceLoader.GetString(L"NotifyIcon_MainWindow");
			AppendMenu(hMenu.get(), MF_STRING, 1, mainWindowText.c_str());

			hstring fmtStr = resourceLoader.GetString(L"NotifyIcon_Timer_Fullscreen");
			std::wstring timerText = fmt::format(
				fmt::runtime(std::wstring_view(fmtStr)),
				AppSettings::Get().CountdownSeconds()
			);
			AppendMenu(hMenu.get(), MF_STRING, 2, timerText.c_str());

			fmtStr = resourceLoader.GetString(L"NotifyIcon_Timer_Windowed");
			timerText = fmt::format(
				fmt::runtime(std::wstring_view(fmtStr)),
				AppSettings::Get().CountdownSeconds()
			);
			AppendMenu(hMenu.get(), MF_STRING, 3, timerText.c_str());

			hstring exitText = resourceLoader.GetString(L"NotifyIcon_Exit");
			AppendMenu(hMenu.get(), MF_STRING, 4, exitText.c_str());

			// hWnd 必须为前台窗口才能正确展示弹出菜单
			// 即使 hWnd 是隐藏的
			SetForegroundWindow(hWnd);

			POINT cursorPos;
			GetCursorPos(&cursorPos);
			BOOL selectedMenuId = TrackPopupMenuEx(
				hMenu.get(),
				TPM_LEFTALIGN | TPM_NONOTIFY | TPM_RETURNCMD,
				cursorPos.x,
				cursorPos.y,
				hWnd,
				nullptr
			);

			switch (selectedMenuId) {
			case 1:
				App::Get().ShowMainWindow();
				break;
			case 2:
				ScalingService::Get().StartTimer(false);
				break;
			case 3:
				ScalingService::Get().StartTimer(true);
				break;
			case 4:
				App::Get().Quit();
				break;
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
		if (message == WM_TASKBARCREATED) {
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
