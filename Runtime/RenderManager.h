#pragma once
#include "pch.h"
#include "Renderable.h"
#include "D2DImageEffectRenderer.h"
#include "WICBitmapEffectRenderer.h"
#include "D2DContext.h"
#include "CursorManager.h"
#include "FrameCatcher.h"
#include "WindowCapturerBase.h"
#include "MagCallbackWindowCapturer.h"
#include "GDIWindowCapturer.h"
#include "WinRTCapturer.h"
#include "Env.h"


// 处理全屏窗口的渲染
class RenderManager {
public:
	RenderManager() {
		// 初始化 D2DContext
		_d2dContext.reset(new D2DContext());

		// 初始化 WindowCapturer
		int captureMode = Env::$instance->GetCaptureMode();
		if (captureMode == 0) {
			_windowCapturer.reset(new WinRTCapturer());
		} else if (captureMode == 1) {
			_windowCapturer.reset(new GDIWindowCapturer());
		} else {
			_windowCapturer.reset(new MagCallbackWindowCapturer());
		}

		// 初始化 EffectRenderer
		CaptureredFrameType frameType = _windowCapturer->GetFrameType();
		if (frameType == CaptureredFrameType::D2DImage) {
			_effectRenderer.reset(new D2DImageEffectRenderer());
		} else {
			_effectRenderer.reset(new WICBitmapEffectRenderer());
		}

		// 初始化 CursorManager
		const RECT& destRect = _effectRenderer->GetOutputRect();
		_cursorManager.reset(new CursorManager(destRect));

		if (Env::$instance->IsShowFPS()) {
			// 初始化 FrameCatcher
			_frameCatcher.reset(new FrameCatcher(destRect));
		}
	}

	std::pair<bool, LRESULT> WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
		return _cursorManager->WndProc(hWnd, message, wParam, lParam);
	}

	void Render() {
		// 每次渲染时检测前台窗口是否改变
		if (GetForegroundWindow() != Env::$instance->GetHwndSrc()) {
			// 前台窗口改变时关闭全屏窗口
			DestroyWindow(Env::$instance->GetHwndHost());
			return;
		}

		try {
			_d2dContext->Render([&](ID2D1DeviceContext* d2dDC) {
				const auto& frame = _windowCapturer->GetFrame();
				if (!frame) {
					return;
				}

				d2dDC->Clear();

				_effectRenderer->SetInput(frame);
				_effectRenderer->Render();

				if (_frameCatcher) {
					_frameCatcher->Render();
				}
				if (_cursorManager) {
					_cursorManager->Render();
				}
				});
		} catch (const magpie_exception& e) {
			Debug::WriteErrorMessage(L"渲染失败：" + e.what());
		} catch (...) {
			Debug::WriteErrorMessage(L"渲染出现未知错误");
		}
	}

private:
	std::unique_ptr<D2DContext> _d2dContext = nullptr;
	std::unique_ptr<WindowCapturerBase> _windowCapturer = nullptr;
	std::unique_ptr<EffectRendererBase> _effectRenderer = nullptr;
	std::unique_ptr<CursorManager> _cursorManager = nullptr;
	std::unique_ptr<FrameCatcher> _frameCatcher = nullptr;
};
