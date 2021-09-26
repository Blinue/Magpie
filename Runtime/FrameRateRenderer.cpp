#include "pch.h"
#include "FrameRateRenderer.h"
#include "App.h"

using namespace std::chrono;


bool FrameRateRenderer::Initialize(ComPtr<ID3D11Texture2D> renderTarget, const RECT& destRect) {
	Renderer& renderer = App::GetInstance().GetRenderer();
	_d3dDC = renderer.GetD3DDC();
	if (!renderer.GetRenderTargetView(renderTarget.Get(), &_rtv)) {
		return false;
	}

	_vp.MaxDepth = 1.0f;
	_vp.TopLeftX = (FLOAT)destRect.left;
	_vp.TopLeftY = (FLOAT)destRect.top;
	_vp.Width = FLOAT(destRect.right - destRect.left);
	_vp.Height = FLOAT(destRect.bottom - destRect.top);

	_spriteBatch.reset(new SpriteBatch(renderer.GetD3DDC().Get()));
	_spriteFont.reset(new SpriteFont(renderer.GetD3DDevice().Get(), L"assets/OpenSans.spritefont"));
	return true;
}

void FrameRateRenderer::Draw() {
	_ReportNewFrame();

	_d3dDC->OMSetRenderTargets(1, &_rtv, nullptr);
	_d3dDC->RSSetViewports(1, &_vp);

	_spriteBatch->Begin(SpriteSortMode::SpriteSortMode_Immediate);

	_spriteFont->DrawString(
		_spriteBatch.get(),
		fmt::format("{} FPS", lround(_fps)).c_str(),
		XMFLOAT2(10, 10)
	);
	_spriteBatch->End();
}

bool FrameRateRenderer::_ReportNewFrame() {
    if (_begin == steady_clock::time_point()) {
        // 第一帧
        _lastBegin = _last = _begin = steady_clock::now();
        return false;
    }

    ++_allFrameCount;

    auto cur = steady_clock::now();
    auto ms = duration_cast<milliseconds>(cur - _lastBegin).count();
    if (ms < 1000) {
        ++_frameCount;
        _last = cur;
        return false;
    }

    // 已过一秒
    _fps = _frameCount + double(duration_cast<milliseconds>(cur - _last).count() + 1000 - ms) / 1000;
    _frameCount += 1 - _fps;
    _lastBegin = cur;

	return true;
}
