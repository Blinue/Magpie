#pragma once
#include "pch.h"
#include <SpriteFont.h>
#include <chrono>


class FrameRateRenderer {
public:
	bool Initialize(ComPtr<ID3D11Texture2D> renderTarget, const RECT& destRect);

	void Draw();

private:
	bool _ReportNewFrame();

	double _GetAvgFPS(std::chrono::steady_clock::time_point cur) {
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(cur - _begin).count();
		return static_cast<double>(_allFrameCount) * 1000 / ms;
	}

	// 用于计算平均帧率
	std::chrono::steady_clock::time_point _begin;	// 第一帧的时间
	int _allFrameCount = 0;	// 已渲染的总帧数


	std::chrono::steady_clock::time_point _lastBegin;	// 保存一秒的开始
	std::chrono::steady_clock::time_point _last;			// 保存上一帧的时间
	double _frameCount = 0;					// 保存一秒内已渲染的帧数，不一定是整数

	double _fps = 0;

	ComPtr<ID3D11DeviceContext3> _d3dDC;
	D3D11_VIEWPORT _vp{};

	ID3D11RenderTargetView* _rtv = nullptr;
	std::unique_ptr<SpriteFont> _spriteFont;
	std::unique_ptr<SpriteBatch> _spriteBatch;
};

