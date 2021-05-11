#pragma once
#include "pch.h"
#include "WindowCapturerBase.h"
#include "Utils.h"


// 使用 GDI 抓取窗口
class GDIWindowCapturer : public WindowCapturerBase {
public:
	GDIWindowCapturer(
		D2DContext& d2dContext,
		HWND hwndSrc,
		const RECT& srcRect,
		ComPtr<IWICImagingFactory2> wicImgFactory,
		bool useBitblt = false
	): _d2dContext(d2dContext), _wicImgFactory(wicImgFactory), _srcRect(srcRect), _hwndSrc(hwndSrc), _useBitblt(useBitblt) {
	}

	ComPtr<IUnknown> GetFrame() override {
		return _useBitblt ? _GetFrameWithBitblt() : _GetFrameWithNoBitblt();
	}

	CaptureredFrameType GetFrameType() override {
		return CaptureredFrameType::WICBitmap;
	}
private:
	ComPtr<IWICBitmapSource> _GetFrameWithNoBitblt() {
		SIZE srcSize = Utils::GetSize(_srcRect);
		RECT windowRect{};
		Debug::ThrowIfWin32Failed(
			GetWindowRect(_hwndSrc, &windowRect),
			L"GetWindowRect失败"
		);

		HDC hdcSrc = GetWindowDC(_hwndSrc);
		Debug::ThrowIfWin32Failed(
			hdcSrc,
			L"GetDC失败"
		);
		// 直接获取要放大窗口的 DC 关联的 HBITMAP，而不是自己创建一个
		HBITMAP hBmpDest = (HBITMAP)GetCurrentObject(hdcSrc, OBJ_BITMAP);
		Debug::ThrowIfWin32Failed(
			hBmpDest,
			L"GetCurrentObject失败"
		);
		Debug::ThrowIfWin32Failed(
			ReleaseDC(_hwndSrc, hdcSrc),
			L"ReleaseDC失败"
		);

		ComPtr<IWICBitmap> wicBmp = nullptr;
		Debug::ThrowIfComFailed(
			_wicImgFactory->CreateBitmapFromHBITMAP(
				hBmpDest,
				NULL,
				WICBitmapAlphaChannelOption::WICBitmapIgnoreAlpha,
				&wicBmp
			),
			L"CreateBitmapFromHBITMAP失败"
		);

		// 裁剪出客户区
		ComPtr<IWICBitmapClipper> wicBmpClipper = nullptr;
		Debug::ThrowIfComFailed(
			_wicImgFactory->CreateBitmapClipper(&wicBmpClipper),
			L"CreateBitmapClipper失败"
		);

		WICRect wicRect = {
			_srcRect.left - windowRect.left,
			_srcRect.top - windowRect.top,
			srcSize.cx,
			srcSize.cy
		};
		Debug::ThrowIfComFailed(
			wicBmpClipper->Initialize(wicBmp.Get(), &wicRect),
			L"wicBmpClipper初始化失败"
		);
		
		return wicBmpClipper;
	}

	ComPtr<IWICBitmapSource> _GetFrameWithBitblt() {
		SIZE srcSize = Utils::GetSize(_srcRect);

		HDC hdcSrc = GetDC(_hwndSrc);
		Debug::ThrowIfWin32Failed(
			hdcSrc,
			L"GetDC失败"
		);
		HDC hdcDest = CreateCompatibleDC(hdcSrc);
		Debug::ThrowIfWin32Failed(
			hdcDest,
			L"CreateCompatibleDC失败"
		);
		HBITMAP hBmpDest = CreateCompatibleBitmap(hdcSrc, srcSize.cx, srcSize.cy);
		Debug::ThrowIfWin32Failed(
			hBmpDest,
			L"CreateCompatibleBitmap失败"
		);

		Debug::ThrowIfWin32Failed(
			SelectObject(hdcDest, hBmpDest),
			L"SelectObject失败"
		);
		Debug::ThrowIfWin32Failed(
			BitBlt(hdcDest, 0, 0, srcSize.cx, srcSize.cy, hdcSrc, 0, 0, SRCCOPY),
			L"BitBlt失败"
		);
		
		Debug::ThrowIfWin32Failed(
			ReleaseDC(_hwndSrc, hdcSrc),
			L"ReleaseDC失败"
		);

		ComPtr<IWICBitmap> wicBmp = nullptr;
		Debug::ThrowIfComFailed(
			_wicImgFactory->CreateBitmapFromHBITMAP(
				hBmpDest,
				NULL,
				WICBitmapAlphaChannelOption::WICBitmapIgnoreAlpha,
				&wicBmp
			),
			L"CreateBitmapFromHBITMAP失败"
		);

		Debug::ThrowIfWin32Failed(
			DeleteBitmap(hBmpDest),
			L"DeleteBitmap失败"
		);
		Debug::ThrowIfWin32Failed(
			DeleteDC(hdcDest),
			L"DeleteDC失败"
		);

		return wicBmp;
	}

	ComPtr<IWICImagingFactory2> _wicImgFactory;
	const RECT& _srcRect;
	HWND _hwndSrc;
	bool _useBitblt;

	D2DContext& _d2dContext;
};
