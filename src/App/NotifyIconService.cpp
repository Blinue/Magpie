#include "pch.h"
#include "NotifyIconService.h"
#include "CommonSharedConstants.h"


namespace winrt::Magpie::App {

static constexpr const wchar_t* NOTIFY_ICON_WINDOW_CLASS_NAME = L"Magpie_NotifyIcon";

// {D0DCEAC7-905E-4955-B660-B9EB815C2139}
static constexpr const GUID NOTIFY_ICON_GUID =
	{ 0xd0dceac7, 0x905e, 0x4955, { 0xb6, 0x60, 0xb9, 0xeb, 0x81, 0x5c, 0x21, 0x39 } };


NotifyIconService::NotifyIconService() {
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

void NotifyIconService::Show() {
	if (_nid.hWnd) {
		return;
	}

	_nid.hWnd = CreateWindow(NOTIFY_ICON_WINDOW_CLASS_NAME, nullptr, WS_OVERLAPPED, 0, 0, 0, 0, NULL, NULL, _hInst, 0);
	LoadIconMetric(_hInst, MAKEINTRESOURCE(CommonSharedConstants::IDI_APP), LIM_SMALL, &_nid.hIcon);
	wcscpy_s(_nid.szTip, std::size(_nid.szTip), L"Magpie");

	Shell_NotifyIcon(NIM_ADD, &_nid);
}

void NotifyIconService::Hide() {
	if (!_nid.hWnd) {
		return;
	}

	Shell_NotifyIcon(NIM_DELETE, &_nid);
	DestroyIcon(_nid.hIcon);
	_nid.hIcon = NULL;

	DestroyWindow(_nid.hWnd);
	_nid.hWnd = NULL;
}

LRESULT NotifyIconService::_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case CommonSharedConstants::WM_NOTIFY_ICON:
	{

		break;
	}
	default:
		break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

}
