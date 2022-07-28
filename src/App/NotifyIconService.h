#pragma once
#include "pch.h"


namespace winrt::Magpie::App {

class NotifyIconService {
public:
	NotifyIconService(const NotifyIconService&) = delete;
	NotifyIconService(NotifyIconService&&) = default;

	static NotifyIconService& Get() {
		static NotifyIconService instance;
		return instance;
	}

	void Show();

	void Hide();

private:
	static LRESULT _WndProcStatic(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		return Get()._WndProc(hWnd, msg, wParam, lParam);
	}
	LRESULT _WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	NotifyIconService();

	HINSTANCE _hInst = NULL;
	NOTIFYICONDATA _nid{};
};

}
