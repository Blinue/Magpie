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

	bool IsWantCaptureMouse() const;

private:
	UINT _handlerID = 0;

	ID3D11RenderTargetView* _rtv = nullptr;

	bool _cursorOnUI = false;
};
