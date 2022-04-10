#pragma once
#include "pch.h"


class OverlayDrawer {
public:
	OverlayDrawer() = default;
	OverlayDrawer(const OverlayDrawer&) = delete;
	OverlayDrawer(OverlayDrawer&&) = delete;

	~OverlayDrawer();

	bool Initialize(ID3D11Texture2D* renderTarget);

	void Draw();

	bool IsVisiable() const noexcept {
		return _isVisiable;
	}

	void SetVisibility(bool value);

private:
	UINT _handlerID = 0;
	ID3D11RenderTargetView* _rtv = nullptr;
	bool _isVisiable = true;
};
