#pragma once
#include "pch.h"
#include "Renderable.h"
#include "D2DImageEffectRenderer.h"
#include "WICBitmapEffectRenderer.h"
#include "D2DContext.h"
#include "CursorManager.h"
#include "FrameCatcher.h"
#include "WindowCapturerBase.h"


class RenderManager {
public:
	RenderManager(
		D2DContext& d2dContext, 
		const std::string_view& scaleModel,
		const RECT& srcClient,
		const RECT& hostClient,
		CaptureredFrameType frameType,
		bool noDisturb = false,
		bool debugMode = false
	) :  _d2dContext(d2dContext), _noDisturb(noDisturb), _debugMode(debugMode),
		_srcClient(srcClient), _hostClient(hostClient)
	{
		if (frameType == CaptureredFrameType::D2DImage) {
			_effectRenderer.reset(new D2DImageEffectRenderer(d2dContext, scaleModel, { srcClient.right - srcClient.left,srcClient.bottom - srcClient.top }, hostClient));
		} else {
			_effectRenderer.reset(new WICBitmapEffectRenderer(d2dContext, scaleModel, { srcClient.right - srcClient.left,srcClient.bottom - srcClient.top }, hostClient));
		}
	}

	void AddCursorManager(HINSTANCE hInst, IWICImagingFactory2* wicImgFactory) {
		const auto& destRect = _effectRenderer->GetOutputRect();

		_cursorManager.reset(new CursorManager(
			_d2dContext,
			hInst,
			wicImgFactory,
			D2D1::RectF((float)_srcClient.left, (float)_srcClient.top, (float)_srcClient.right, (float)_srcClient.bottom),
			D2D1::RectF((float)destRect.left, (float)destRect.top, (float)destRect.right, (float)destRect.bottom),
			_noDisturb,
			_debugMode
		));
	}

	void AddHookCursor(HCURSOR hTptCursor, HCURSOR hCursor) {
		assert(_cursorManager);
		_cursorManager->AddHookCursor(hTptCursor, hCursor);
	}

	void AddFrameCatcher() {
		_frameCatcher.reset(new FrameCatcher(_d2dContext, _GetDWFactory(), _effectRenderer->GetOutputRect()));
	}

	void Render(ComPtr<IUnknown> frame) {
		assert(frame);

		_d2dContext.Render([&]() {
			_d2dContext.GetD2DDC()->Clear();

			_effectRenderer->SetInput(frame);
			_effectRenderer->Render();

			if (_frameCatcher) {
				_frameCatcher->Render();
			}
			if (_cursorManager) {
				_cursorManager->Render();
			}
		});
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
				L"´´½¨ IDWriteFactory Ê§°Ü"
			);
		}

		return _dwFactory.Get();
	}

	ComPtr<IDWriteFactory> _dwFactory = nullptr;

	std::unique_ptr<EffectRendererBase> _effectRenderer = nullptr;
	std::unique_ptr<CursorManager> _cursorManager = nullptr;
	std::unique_ptr<FrameCatcher> _frameCatcher = nullptr;

	D2DContext& _d2dContext;

	const RECT& _srcClient;
	const RECT& _hostClient;

	bool _noDisturb;
	bool _debugMode;
};
