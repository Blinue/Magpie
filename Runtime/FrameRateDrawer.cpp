#include "pch.h"
#include "FrameRateDrawer.h"
#include "App.h"

using namespace std::chrono;


bool FrameRateDrawer::Initialize(ComPtr<ID3D11Texture2D> renderTarget, const RECT& destRect) {
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

void FrameRateDrawer::Draw() {
	const StepTimer& timer = App::GetInstance().GetRenderer().GetTimer();

	_d3dDC->OMSetRenderTargets(1, &_rtv, nullptr);
	_d3dDC->RSSetViewports(1, &_vp);

	_spriteBatch->Begin(SpriteSortMode::SpriteSortMode_Immediate);
	_spriteFont->DrawString(
		_spriteBatch.get(),
		fmt::format("{} FPS", timer.GetFramesPerSecond()).c_str(),
		XMFLOAT2(10, 10)
	);
	_spriteBatch->End();
}
