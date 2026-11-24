#pragma once
#include "WindowBase.h"

class CursorWindow : public WindowBaseT<CursorWindow> {
	using base_type = WindowBaseT<CursorWindow>;
	friend base_type;

public:
	bool Create() noexcept;

private:
	LRESULT _MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

	void _UpdateButtonPos() noexcept;

	void _UpdateStandardCursor(bool useStandardCursor) noexcept;

	wil::unique_hcursor _hCursor = NULL;
	uint32_t _curStandardCursorIdx = std::numeric_limits<uint32_t>::max();

	HWND _hwndBtn1 = NULL;
	HWND _hwndBtn2 = NULL;
	HWND _hwndBtn3 = NULL;
	HWND _hwndBtn4 = NULL;
};
