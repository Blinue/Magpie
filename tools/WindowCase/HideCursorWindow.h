#pragma once
#include "WindowBase.h"

class HideCursorWindow : public WindowBaseT<HideCursorWindow> {
	using base_type = WindowBaseT<HideCursorWindow>;
	friend base_type;

public:
	bool Create(HINSTANCE hInst) noexcept;

private:
	LRESULT _MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

	void _UpdateButtonPos() noexcept;

	HWND _hwndBtn = NULL;
	bool _isCursorHidden = false;
};
