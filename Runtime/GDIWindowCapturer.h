#pragma once
#include "pch.h"
#include "WindowCapturerBase.h"
#include "Utils.h"
#include <queue>
#include <chrono>

using namespace std::chrono;


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
		// 在单独的线程中不断使用GDI截获窗口
		// 如果放在渲染线程中会造成卡顿
		_getFrameThread.reset(new std::thread([&]() {
			while (!_closed) {
				ComPtr<IWICBitmapSource> frame = _GetFrameWithNoBitblt();

				_frameMutex.lock();
				_frame = frame;
				_frameMutex.unlock();

				std::this_thread::yield();
			}
		}));
	}

	~GDIWindowCapturer() {
		_closed = true;
		_getFrameThread->join();
	}

	ComPtr<IUnknown> GetFrame() override {
		std::lock_guard<std::mutex> guard(_frameMutex);

		ComPtr<IWICBitmapSource> frame = _frame;
		_frame = nullptr;
		return frame;
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

	ComPtr<IWICImagingFactory2> _wicImgFactory;
	const RECT& _srcRect;
	const HWND _hwndSrc;
	bool _useBitblt;

	D2DContext& _d2dContext;

	ComPtr<IWICBitmapSource> _frame;
	// 同步对 _frame 的访问
	std::mutex _frameMutex;
	std::atomic_bool _closed = false;

	std::unique_ptr<std::thread> _getFrameThread;
};
