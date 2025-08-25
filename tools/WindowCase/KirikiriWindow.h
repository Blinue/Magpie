#pragma once
#include "WindowBase.h"

class KirikiriWindow : public WindowBaseT<KirikiriWindow> {
	using base_type = WindowBaseT<KirikiriWindow>;
	friend base_type;

public:
	bool Create() noexcept;

protected:
	LRESULT _MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

private:
	static LRESULT CALLBACK _OwnerWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	LRESULT _PopupMessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

	void _UpdateButtonPos() noexcept;

	HWND _hwndOwner = NULL;
	HWND _hwndBtn1 = NULL;
	HWND _hwndBtn2 = NULL;

	std::unique_ptr<KirikiriWindow> _popup1;
	std::unique_ptr<KirikiriWindow> _popup2;
	// 下面三个成员用于弹窗
	HWND _hwndMain = NULL;
	bool _isPopup = false;
	bool _isOwnedPopup = false;
};
