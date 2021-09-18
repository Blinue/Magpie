#include "pch.h"
#include "FrameRateRenderer.h"
#include "App.h"

using namespace std::chrono;


bool FrameRateRenderer::Initialize() {
	Renderer& renderer = App::GetInstance().GetRenderer();
	_spriteBatch.reset(new SpriteBatch(App::GetInstance().GetRenderer().GetD3DDC().Get()));

	return true;
}

void FrameRateRenderer::Draw() {
	if (_ReportNewFrame()) {
		OutputDebugString(fmt::format(L"{} FPS\n", lround(_fps)).c_str());
	}
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
