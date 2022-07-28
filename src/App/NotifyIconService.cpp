#include "pch.h"
#include "NotifyIconService.h"


namespace winrt::Magpie::App {

static constexpr const wchar_t* NOTIFY_ICON_WINDOW_CLASS_NAME = L"Magpie_NotifyIcon";

NotifyIconService::NotifyIconService() {
	_hInst = GetModuleHandle(nullptr);

	WNDCLASSEXW wcex{};
	wcex.cbSize = sizeof(wcex);
	wcex.lpfnWndProc = _WndProcStatic;
	wcex.hInstance = _hInst;
	wcex.lpszClassName = NOTIFY_ICON_WINDOW_CLASS_NAME;

	RegisterClassEx(&wcex);
}

void NotifyIconService::Show() {
	if (!_hWnd) {
		_hWnd = CreateWindow(NOTIFY_ICON_WINDOW_CLASS_NAME, nullptr, WS_OVERLAPPED, 0, 0, 0, 0, NULL, NULL, _hInst, 0);
	}

	NOTIFYICONDATA data{};
	data.cbSize = sizeof(data);

	Shell_NotifyIcon(NIM_ADD, &data);
}

void NotifyIconService::Hide() {
}

LRESULT NotifyIconService::_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	return LRESULT();
}

}
