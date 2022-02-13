#pragma once
#include "pch.h"
#include <SpriteFont.h>
#include <chrono>


class FrameRateDrawer {
public:
	FrameRateDrawer() = default;
	FrameRateDrawer(const FrameRateDrawer&) = delete;
	FrameRateDrawer(FrameRateDrawer&&) = delete;

	bool Initialize(ID3D11Texture2D* renderTarget, const RECT& destRect);

	void Draw();

private:
	D3D11_VIEWPORT _vp{};

	ID3D11RenderTargetView* _rtv = nullptr;
	std::unique_ptr<SpriteFont> _spriteFont;
	std::unique_ptr<SpriteBatch> _spriteBatch;
};
