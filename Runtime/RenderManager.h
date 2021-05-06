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


class RenderManager {
public:
	RenderManager(
		HINSTANCE hInst,
		const std::string_view& scaleModel,
		HWND hwndSrc,
		HWND hwndHost,
		int captureMode,
		bool showFPS,
		bool lowLatencyMode,
		bool noVSync,
		bool noDisturb,
		bool debugMode = false
	) :  _noDisturb(noDisturb), _debugMode(debugMode), _hInst(hInst), _hwndSrc(hwndSrc), _hwndHost(hwndHost)
	{
		Debug::Assert(
			captureMode >= 0 && captureMode <= 2,
			L"非法的抓取模式"
		);

		Utils::GetClientScreenRect(_hwndSrc, _srcClient);
		Utils::GetClientScreenRect(_hwndHost, _hostClient);

		Debug::ThrowIfComFailed(
			CoCreateInstance(
				CLSID_WICImagingFactory,
				NULL,
				CLSCTX_INPROC_SERVER,
				IID_PPV_ARGS(&_wicImgFactory)
			),
			L"创建 WICImagingFactory 失败"
		);

		// 初始化 D2DContext
		_d2dContext.reset(new D2DContext(
			_hInst,
			_hwndHost,
			_hostClient,
			lowLatencyMode,
			noVSync,
			noDisturb
		));

		// 初始化 WindowCapturer
		if (captureMode == 0) {
			_windowCapturer.reset(new WinRTCapturer(
				*_d2dContext,
				_hwndSrc,
				_srcClient
			));
		} else if (captureMode == 1) {
			_windowCapturer.reset(new GDIWindowCapturer(
				*_d2dContext,
				_hwndSrc,
				_srcClient,
				_wicImgFactory
			));
		} else {
			_windowCapturer.reset(new MagCallbackWindowCapturer(
				*_d2dContext,
				_hInst,
				_hwndHost,
				_srcClient
			));
		}

		// 初始化 EffectRenderer
		CaptureredFrameType frameType = _windowCapturer->GetFrameType();
		if (frameType == CaptureredFrameType::D2DImage) {
			_effectRenderer.reset(new D2DImageEffectRenderer(*_d2dContext, scaleModel, { _srcClient.right - _srcClient.left,_srcClient.bottom - _srcClient.top }, _hostClient));
		} else {
			_effectRenderer.reset(new WICBitmapEffectRenderer(*_d2dContext, scaleModel, { _srcClient.right - _srcClient.left,_srcClient.bottom - _srcClient.top }, _hostClient));
		}

		// 初始化 CursorManager
		const auto& destRect = _effectRenderer->GetOutputRect();
		_cursorManager.reset(new CursorManager(
			*_d2dContext,
			_hInst,
			_wicImgFactory.Get(),
			D2D1::RectF((float)_srcClient.left, (float)_srcClient.top, (float)_srcClient.right, (float)_srcClient.bottom),
			D2D1::RectF((float)destRect.left, (float)destRect.top, (float)destRect.right, (float)destRect.bottom),
			_noDisturb,
			_debugMode
		));

		if (showFPS) {
			// 初始化 FrameCatcher
			_frameCatcher.reset(new FrameCatcher(*_d2dContext, _GetDWFactory(), _effectRenderer->GetOutputRect()));
		}

		if (_windowCapturer->GetCaptureStyle() == CaptureStyle::Event) {
			_windowCapturer->On([&]() {
				_RenderNextFrame();
			});

			// CaptureStyle 为 Event 时，为了增强窗口响应性，
			// 每 100 毫秒检测前台窗口
			Debug::ThrowIfWin32Failed(
				SetTimer(_hwndHost, _CHECK_FOREGROUND_TIMER_ID, 100, nullptr),
				L"SetTimer失败"
			);
		} else {
			// 立即渲染第一帧
			_RenderNextFrame();
		}
	}

	std::pair<bool, LRESULT> WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
		if(message == WM_TIMER) {
			if (wParam == _CHECK_FOREGROUND_TIMER_ID) {
				_CheckForeground();
				return { true, 0 };
			}
		} else if (message == _WM_RENDER) {
			if (!_CheckForeground()) {
				_Render();

				if (_windowCapturer->GetCaptureStyle() == CaptureStyle::Normal) {
					// 立即渲染下一帧
					// 垂直同步开启时自动限制帧率
					_RenderNextFrame();
				}
			}

			return { true, 0 };
		}

		return _cursorManager->WndProc(hWnd, message, wParam, lParam);
	}
private:
	IDWriteFactory* _GetDWFactory() {
		if (_dwFactory == nullptr) {
			Debug::ThrowIfComFailed(
				DWriteCreateFactory(
					DWRITE_FACTORY_TYPE_SHARED,
					__uuidof(IDWriteFactory),
					&_dwFactory
				),
				L"创建 IDWriteFactory 失败"
			);
		}

		return _dwFactory.Get();
	}

	void _RenderNextFrame() {
		if (!PostMessage(_hwndHost, _WM_RENDER, 0, 0)) {
			Debug::WriteErrorMessage(L"PostMessage 失败");
		}
	}

	// 渲染一帧，不抛出异常
	void _Render() {
		try {
			const auto& frame = _windowCapturer->GetFrame();
			if (!frame) {
				return;
			}

			_d2dContext->Render([&]() {
				_d2dContext->GetD2DDC()->Clear();

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

	// 前台窗口改变时自动关闭全屏窗口
	// 如果前台窗口已改变，返回 true，否则返回 false
	bool _CheckForeground() {
		bool r = GetForegroundWindow() != _hwndSrc;
		if (r) {
			DestroyWindow(_hwndHost);
		}

		return r;
	}


	ComPtr<IDWriteFactory> _dwFactory = nullptr;

	std::unique_ptr<D2DContext> _d2dContext = nullptr;
	std::unique_ptr<WindowCapturerBase> _windowCapturer = nullptr;
	std::unique_ptr<EffectRendererBase> _effectRenderer = nullptr;
	std::unique_ptr<CursorManager> _cursorManager = nullptr;
	std::unique_ptr<FrameCatcher> _frameCatcher = nullptr;

	HWND _hwndSrc;
	HWND _hwndHost;
	RECT _srcClient;
	RECT _hostClient;

	bool _noDisturb;
	bool _debugMode;

	static UINT _WM_RENDER;

	static constexpr UINT_PTR _CHECK_FOREGROUND_TIMER_ID = 1;

	ComPtr<IWICImagingFactory2> _wicImgFactory = nullptr;
	HINSTANCE _hInst;
};
