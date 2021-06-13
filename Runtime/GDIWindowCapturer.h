#pragma once
#include "pch.h"
#include "WindowCapturerBase.h"
#include "Utils.h"
#include "Env.h"


// 使用 GDI 抓取窗口
class GDIWindowCapturer : public WindowCapturerBase {
public:
	ComPtr<IUnknown> GetFrame() override {
		HWND hwndSrc = Env::$instance->GetHwndSrc();

		RECT windowRect{};
		Debug::ThrowIfWin32Failed(
			GetWindowRect(hwndSrc, &windowRect),
			L"GetWindowRect失败"
		);

		HDC hdcSrc = GetWindowDC(hwndSrc);
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
			ReleaseDC(hwndSrc, hdcSrc),
			L"ReleaseDC失败"
		);

		IWICImagingFactory2* wicImgFactory = Env::$instance->GetWICImageFactory();
		ComPtr<IWICBitmap> wicBmp = nullptr;
		Debug::ThrowIfComFailed(
			wicImgFactory->CreateBitmapFromHBITMAP(
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
			wicImgFactory->CreateBitmapClipper(&wicBmpClipper),
			L"CreateBitmapClipper失败"
		);

		const RECT& srcClient = Env::$instance->GetSrcClient();
		WICRect wicRect = {
			srcClient.left - windowRect.left,
			srcClient.top - windowRect.top,
			srcClient.right - srcClient.left,
			srcClient.bottom - srcClient.top
		};
		Debug::ThrowIfComFailed(
			wicBmpClipper->Initialize(wicBmp.Get(), &wicRect),
			L"wicBmpClipper初始化失败"
		);

		return wicBmpClipper;
	}

	CaptureredFrameType GetFrameType() override {
		return CaptureredFrameType::WICBitmap;
	}
};
