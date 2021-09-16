#pragma once
#include "pch.h"
#include <SpriteFont.h>
#include "App.h"


class FrameRateRenderer {
public:
	bool Initialize() {
		_spriteBatch.reset(new SpriteBatch(App::GetInstance().GetRenderer().GetD3DDC().Get()));
		
	}

private:
	std::unique_ptr<SpriteFont> _spriteFont;
	std::unique_ptr<SpriteBatch> _spriteBatch;
};

