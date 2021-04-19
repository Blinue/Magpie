#pragma once
#include "pch.h"
#include "Renderable.h"
#include "EffectManager.h"
#include "D2DContext.h"
#include "CursorManager.h"
#include "FrameCatcher.h"


class RenderManager {
public:
	RenderManager(
		D2DContext& d2dContext, 
		const std::wstring_view& effectsJson,
		const RECT& srcClient,
		const RECT& hostClient,
		bool noDisturb = false,
		bool debugMode = false
	) :  _d2dContext(d2dContext),
		_noDisturb(noDisturb), _debugMode(debugMode), _srcClient(srcClient), _hostClient(hostClient),
		_effectManager(new EffectManager(d2dContext, effectsJson, { srcClient.right- srcClient.left,srcClient.bottom- srcClient.top}, hostClient))
	{
	}

	void AddCursorManager(HINSTANCE hInst, IWICImagingFactory2* wicImgFactory) {
		const auto& destRect = _effectManager->GetOutputRect();

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
		_frameCatcher.reset(new FrameCatcher(_d2dContext, _GetDWFactory(), _effectManager->GetOutputRect()));
	}

	void Render(
		const std::variant<ComPtr<IWICBitmapSource>, ComPtr<ID2D1Bitmap1>>& frame,
		const RECT& srcRect,
		const RECT& srcClient
	) {
		if (frame.index() == 0) {
			_d2dContext.Render([&]() {
				_d2dContext.GetD2DDC()->Clear();

				_effectManager->SetInput(std::get<0>(frame).Get());
				_effectManager->Render();

				if (_frameCatcher) {
					_frameCatcher->Render();
				}
				if (_cursorManager) {
					_cursorManager->Render();
				}
				});
		} else {
			const auto& f = std::get<1>(frame);
			if (!f) {
				return;
			}

			_d2dContext.Render([&]() {
				_d2dContext.GetD2DDC()->Clear();

				RECT rect{};
				
				_effectManager->SetInput(
					f.Get(),
					{
						srcClient.left - srcRect.left,
						srcClient.top - srcRect.top,
						srcClient.right - srcRect.left,
						srcClient.bottom - srcRect.top
					}
				);
				_effectManager->Render();

				if (_frameCatcher) {
					_frameCatcher->Render();
				}
				if (_cursorManager) {
					_cursorManager->Render();
				}
				});
		}

		
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

	std::unique_ptr<EffectManager> _effectManager = nullptr;
	std::unique_ptr<CursorManager> _cursorManager = nullptr;
	std::unique_ptr<FrameCatcher> _frameCatcher = nullptr;

	D2DContext& _d2dContext;

	const RECT& _srcClient;
	const RECT& _hostClient;

	bool _noDisturb;
	bool _debugMode;
};
