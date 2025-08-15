#pragma once
#include "WindowBase.h"

class HungWindow : public WindowBaseT<HungWindow> {
	using base_type = WindowBaseT<HungWindow>;
	friend base_type;

public:
	bool Create(HINSTANCE hInst) noexcept;

protected:
	LRESULT _MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

private:
	void _UpdateButtonPos() noexcept;

	HWND _hwndBtn = NULL;
	HFONT _hUIFont = NULL;
};
