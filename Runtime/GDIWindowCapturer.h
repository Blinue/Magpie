#pragma once
#include "pch.h"
#include "WindowCapturerBase.h"
#include "Utils.h"
#include <queue>
#include <chrono>
#include "Env.h"

using namespace std::chrono;


// 使用 GDI 抓取窗口
class GDIWindowCapturer : public WindowCapturerBase {
public:
	GDIWindowCapturer() {
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


	ComPtr<IWICBitmapSource> _frame;
	// 同步对 _frame 的访问
	std::mutex _frameMutex;
	std::atomic_bool _closed = false;

	std::unique_ptr<std::thread> _getFrameThread;
};
