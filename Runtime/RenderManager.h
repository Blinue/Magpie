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
			throw new magpie_exception(L"非法的抓取模式");
		}

		// 初始化 EffectRenderer
		CaptureredFrameType frameType = _windowCapturer->GetFrameType();
		if (frameType == CaptureredFrameType::D2DImage) {
			_effectRenderer.reset(new D2DImageEffectRenderer());
		} else {
			_effectRenderer.reset(new WICBitmapEffectRenderer());
		}

		// 初始化 CursorManager
		_cursorManager.reset(new CursorManager());

		if (Env::$instance->IsShowFPS()) {
			// 初始化 FrameCatcher
			_frameCatcher.reset(new FrameCatcher());
		}
	}

	std::pair<bool, LRESULT> WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
		return _cursorManager->WndProc(hWnd, message, wParam, lParam);
	}

	void Render() {
		// 每次渲染之前检测源窗口状态
		if (!_CheckSrcState()) {
			return;
		}
		const auto& frame = _windowCapturer->GetFrame();
		if (!frame) {
			return;
		}

		_d2dContext->Render([&](ID2D1DeviceContext* d2dDC) {
			d2dDC->Clear();

			ComPtr<ID2D1Image> img = _effectRenderer->Apply(frame.Get());
			img = _cursorManager->RenderEffect(img);
			
			const D2D_RECT_F& destRect = Env::$instance->GetDestRect();
			Env::$instance->GetD2DDC()->DrawImage(img.Get(), { destRect.left, destRect.top });

			if (_frameCatcher) {
				_frameCatcher->Render();
			}
			if (_cursorManager) {
				_cursorManager->Render();
			}
		});
	}

	
private:
	bool _CheckSrcState() {
		HWND hwndSrc = Env::$instance->GetHwndSrc();
		if (GetForegroundWindow() == hwndSrc) {
			// 先检查前台窗口，否则在窗口已关闭时GetClientScreenRect会出错
			RECT rect;
			Utils::GetClientScreenRect(hwndSrc, rect);

			if (Env::$instance->GetSrcClient() == rect && Utils::GetWindowShowCmd(hwndSrc) == SW_NORMAL) {
				return true;
			}
		}

		// 状态改变时关闭全屏窗口
		DestroyWindow(Env::$instance->GetHwndHost());
		return false;
	}

	std::unique_ptr<D2DContext> _d2dContext = nullptr;
	std::unique_ptr<WindowCapturerBase> _windowCapturer = nullptr;
	std::unique_ptr<EffectRendererBase> _effectRenderer = nullptr;
	std::unique_ptr<CursorManager> _cursorManager = nullptr;
	std::unique_ptr<FrameCatcher> _frameCatcher = nullptr;
};
