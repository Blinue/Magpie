#pragma once
#include "WindowBase.h"

class PopupHostWindow : public WindowBaseT<PopupHostWindow> {
	using base_type = WindowBaseT<PopupHostWindow>;
	friend base_type;

public:
	bool Create(HINSTANCE hInst) noexcept;

private:
	LRESULT _MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

	void _UpdateButtonPos() noexcept;

	HWND _hwndBtn1 = NULL;
	HWND _hwndBtn2 = NULL;
	HWND _hwndBtn3 = NULL;
	HWND _hwndBtn4 = NULL;
};
