#pragma once
#include "WindowBase.h"

class TopmostWindow : public WindowBaseT<TopmostWindow> {
	using base_type = WindowBaseT<TopmostWindow>;
	friend base_type;

public:
	bool Create() noexcept;

private:
	LRESULT _MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

	void _UpdateButtonPos() noexcept;

	HWND _hwndBtn = NULL;
};
