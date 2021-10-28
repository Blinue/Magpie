#pragma once
#include "pch.h"
#include <SpriteFont.h>
#include <chrono>


class FrameRateDrawer {
public:
	bool Initialize(ComPtr<ID3D11Texture2D> renderTarget, const RECT& destRect);

	void Draw();

private:
	ComPtr<ID3D11DeviceContext> _d3dDC;
	D3D11_VIEWPORT _vp{};

	ID3D11RenderTargetView* _rtv = nullptr;
	std::unique_ptr<SpriteFont> _spriteFont;
	std::unique_ptr<SpriteBatch> _spriteBatch;
};

