#pragma once
#include "WindowBase.h"

class KirikiriWindow : public WindowBaseT<KirikiriWindow> {
	using base_type = WindowBaseT<KirikiriWindow>;
	friend base_type;

public:
	bool Create(HINSTANCE hInst) noexcept;

protected:
	LRESULT _MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept;
};
