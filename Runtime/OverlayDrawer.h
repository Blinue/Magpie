#pragma once
#include "pch.h"

struct ImFont;

class OverlayDrawer {
public:
	OverlayDrawer() = default;
	OverlayDrawer(const OverlayDrawer&) = delete;
	OverlayDrawer(OverlayDrawer&&) = delete;

	~OverlayDrawer();

	bool Initialize(ID3D11Texture2D* renderTarget);

	void Draw();

	bool IsUIVisiable() const noexcept {
		return _isUIVisiable;
	}

	void SetUIVisibility(bool value);

private:
	void _DrawFPS();

	void _DrawUI();

	UINT _handlerID = 0;
	ID3D11RenderTargetView* _rtv = nullptr;
	bool _isUIVisiable = false;

	ImFont* _fontSmall = nullptr;
	ImFont* _fontLarge = nullptr;
};
