#pragma once
#include "pch.h"


class UIDrawer {
public:
	UIDrawer() = default;
	UIDrawer(const UIDrawer&) = delete;
	UIDrawer(UIDrawer&&) = delete;

	~UIDrawer();

	bool Initialize(ID3D11Texture2D* renderTarget);

	void Draw();

private:
	static bool _WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	UINT _handlerID = 0;

	ID3D11RenderTargetView* _rtv = nullptr;

	bool _cursorOnUI = false;
};
