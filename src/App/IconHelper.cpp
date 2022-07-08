#include "pch.h"
#include "IconHelper.h"
#include "Logger.h"
#include "Utils.h"


using namespace winrt;
using namespace Windows::Graphics::Imaging;


namespace winrt::Magpie::App {

IAsyncOperation<ImageSource> IconHelper::HIcon2ImageSourceAsync(HICON hIcon) {
	// 单色图标：不处理
	// 彩色掩码图标：忽略掩码

	ICONINFO ii{};
	if (!GetIconInfo(hIcon, &ii)) {
		Logger::Get().Win32Error("GetIconInfo 失败");
		co_return nullptr;
	}

	Utils::ScopeExit se([&] {
		if (ii.hbmColor) {
			DeleteBitmap(ii.hbmColor);
		}
		if (ii.hbmMask) {
			DeleteBitmap(ii.hbmMask);
		}
	});

	if (!ii.fIcon) {
		co_return nullptr;
	}

	BITMAP bmp{};
	GetObject(ii.hbmColor, sizeof(BITMAP), &bmp);

	SoftwareBitmap bitmap(BitmapPixelFormat::Bgra8, bmp.bmWidth, bmp.bmHeight, BitmapAlphaMode::Premultiplied);
	{
		BitmapBuffer buffer = bitmap.LockBuffer(BitmapBufferAccessMode::Write);
		uint8_t* pixels = buffer.CreateReference().data();
		
		BITMAPINFO bi{};
		bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bi.bmiHeader.biWidth = bmp.bmWidth;
		bi.bmiHeader.biHeight = -bmp.bmHeight;
		bi.bmiHeader.biPlanes = 1;
		bi.bmiHeader.biCompression = BI_RGB;
		bi.bmiHeader.biBitCount = 32;
		bi.bmiHeader.biSizeImage = bmp.bmWidth * bmp.bmHeight * 4;

		HDC hdc = GetDC(NULL);
		if (GetDIBits(hdc, ii.hbmColor, 0, bmp.bmHeight, pixels, &bi, DIB_RGB_COLORS) != bmp.bmHeight) {
			Logger::Get().Win32Error("GetDIBits 失败");
			ReleaseDC(NULL, hdc);
			co_return nullptr;
		}
		ReleaseDC(NULL, hdc);

		// 若颜色掩码有 A 通道，则是彩色图标，否则是彩色掩码图标
		bool hasAlpha = false;
		for (UINT i = 3; i < bi.bmiHeader.biSizeImage; i += 4) {
			if (pixels[i] != 0) {
				hasAlpha = true;
				break;
			}
		}

		if (hasAlpha) {
			// 彩色图标
			for (size_t i = 0; i < bi.bmiHeader.biSizeImage; i += 4) {
				// 预乘 Alpha 通道
				double alpha = pixels[i + 3] / 255.0f;

				pixels[i] = (BYTE)std::lround(pixels[i] * alpha);
				pixels[i + 1] = (BYTE)std::lround(pixels[i + 1] * alpha);
				pixels[i + 2] = (BYTE)std::lround(pixels[i + 2] * alpha);
			}
		} else {
			// 彩色掩码图标
			for (size_t i = 0; i < bi.bmiHeader.biSizeImage; i += 4) {
				pixels[i + 3] = 255;
			}
		}
	}
	
	Imaging::SoftwareBitmapSource result;
	co_await result.SetBitmapAsync(bitmap);
	co_return result;
}

}