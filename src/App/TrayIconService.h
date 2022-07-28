#pragma once
#include "pch.h"


namespace winrt::Magpie::App {

class TrayIconService {
public:
	TrayIconService(const TrayIconService&) = delete;
	TrayIconService(TrayIconService&&) = default;

	static TrayIconService& Get() {
		static TrayIconService instance;
		return instance;
	}

	void Show();

	void Hide();

private:
	static LRESULT _WndProcStatic(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		return Get()._WndProc(hWnd, msg, wParam, lParam);
	}
	LRESULT _WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	TrayIconService();

	HINSTANCE _hInst = NULL;
	NOTIFYICONDATA _nid{};
};

}
