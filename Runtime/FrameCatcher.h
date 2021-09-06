#pragma once
#include "pch.h"
#include "Renderable.h"
#include <chrono>
using namespace std::chrono;


// 计算帧率
class FrameCatcher : public Renderable {
public:
	FrameCatcher() {
		Debug::ThrowIfComFailed(
			Env::$instance->GetDWFactory()->CreateTextFormat(
				L"Microsoft YaHei",
				nullptr,
				DWRITE_FONT_WEIGHT_REGULAR,
				DWRITE_FONT_STYLE_NORMAL,
				DWRITE_FONT_STRETCH_NORMAL,
				20,
				L"en-us",
				&_dwTxtFmt
			),
			L"创建IDWriteTextFormat失败"
		);

		Debug::ThrowIfComFailed(
			Env::$instance->GetD2DDC()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &_d2dFPSTxtBrush),
			L"创建 _d2dFPSTxtBrush 失败"
		);
		Debug::ThrowIfComFailed(
			Env::$instance->GetD2DDC()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::DimGray), &_d2dFPSTxtShadowBrush),
			L"创建 _d2dFPSTxtShadowBrush 失败"
		);
	}

	void Render() override {
		_ReportNewFrame();

		const D2D_RECT_F& destRect = Env::$instance->GetDestRect();
		std::wstring fps = fmt::format(L"{} FPS", lround(_fps));
		// 绘制阴影
		Env::$instance->GetD2DDC()->DrawTextW(
			fps.c_str(),
			(UINT32)fps.size(),
			_dwTxtFmt.Get(),
			D2D1::RectF(
				destRect.left + 12,
				destRect.top + 12,
				destRect.right + 2,
				destRect.bottom + 2
			),
			_d2dFPSTxtShadowBrush.Get()
		);
		// 绘制文本
		Env::$instance->GetD2DDC()->DrawTextW(
			fps.c_str(),
			(UINT32)fps.size(),
			_dwTxtFmt.Get(),
			D2D1::RectF(
				destRect.left + 10,
				destRect.top + 10,
				destRect.right,
				destRect.bottom
			),
			_d2dFPSTxtBrush.Get()
		);
		//Debug::WriteLine(_fps);
		//Debug::WriteLine(L"AVG: "s + std::to_wstring(_GetAvgFPS(cur)));
	}

private:
	void _ReportNewFrame() {
		if (_begin == steady_clock::time_point()) {
			// 第一帧
			_lastBegin = _last = _begin = steady_clock::now();
			return;
		}

		++_allFrameCount;

		auto cur = steady_clock::now();
		auto ms = duration_cast<milliseconds>(cur - _lastBegin).count();
		if (ms < 1000) {
			++_frameCount;
			_last = cur;
			return;
		}

		// 已过一秒
		_fps = _frameCount + double(duration_cast<milliseconds>(cur - _last).count() + 1000 - ms) / 1000;
		_frameCount += 1 - _fps;
		_lastBegin = cur;
	}

	double _GetAvgFPS(steady_clock::time_point cur) {
		auto ms = duration_cast<milliseconds>(cur - _begin).count();
		return static_cast<double>(_allFrameCount) * 1000 / ms;
	}

	// 用于计算平均帧率
	steady_clock::time_point _begin;	// 第一帧的时间
	int _allFrameCount = 0;	// 已渲染的总帧数


	steady_clock::time_point _lastBegin;	// 保存一秒的开始
	steady_clock::time_point _last;			// 保存上一帧的时间
	double _frameCount = 0;					// 保存一秒内已渲染的帧数，不一定是整数
	
	double _fps = 0;

	ComPtr<IDWriteTextFormat> _dwTxtFmt = nullptr;
	ComPtr<ID2D1SolidColorBrush> _d2dFPSTxtBrush = nullptr;
	ComPtr<ID2D1SolidColorBrush> _d2dFPSTxtShadowBrush = nullptr;
};
