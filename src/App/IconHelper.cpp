#include "pch.h"
#include "IconHelper.h"
#include "Logger.h"
#include "Utils.h"
#include "Win32Utils.h"
#include "StrUtils.h"
#include <shellapi.h>

using namespace winrt;
using namespace Windows::Graphics::Imaging;
using namespace Windows::UI::Xaml::Media::Imaging;


namespace winrt::Magpie::App {

static bool CopyPixelsOfHBmp(HBITMAP hBmp, LONG width, LONG height, void* data) {
	BITMAPINFO bi{};
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth = width;
	bi.bmiHeader.biHeight = -height;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biCompression = BI_RGB;
	bi.bmiHeader.biBitCount = 32;
	bi.bmiHeader.biSizeImage = width * height * 4;

	HDC hdc = GetDC(NULL);
	if (GetDIBits(hdc, hBmp, 0, height, data, &bi, DIB_RGB_COLORS) != height) {
		Logger::Get().Win32Error("GetDIBits 失败");
		ReleaseDC(NULL, hdc);
		return false;
	}

	ReleaseDC(NULL, hdc);
	return true;
}

static SoftwareBitmap HIcon2SoftwareBitmapAsync(HICON hIcon) {
	// 单色图标：不处理
	// 彩色掩码图标：忽略掩码

	ICONINFO ii{};
	if (!GetIconInfo(hIcon, &ii)) {
		Logger::Get().Win32Error("GetIconInfo 失败");
		return nullptr;
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
		return nullptr;
	}

	BITMAP bmp{};
	GetObject(ii.hbmColor, sizeof(BITMAP), &bmp);

	SoftwareBitmap bitmap(BitmapPixelFormat::Bgra8, bmp.bmWidth, bmp.bmHeight, BitmapAlphaMode::Premultiplied);
	{
		BitmapBuffer buffer = bitmap.LockBuffer(BitmapBufferAccessMode::Write);
		uint8_t* pixels = buffer.CreateReference().data();
		
		if (!CopyPixelsOfHBmp(ii.hbmColor, bmp.bmWidth, bmp.bmHeight, pixels)) {
			return nullptr;
		}

		const UINT pixelsSize = bmp.bmWidth * bmp.bmHeight * 4;

		// 若颜色掩码有 A 通道，则是彩色图标，否则是彩色掩码图标
		bool hasAlpha = false;
		for (UINT i = 3; i < pixelsSize; i += 4) {
			if (pixels[i] != 0) {
				hasAlpha = true;
				break;
			}
		}

		if (hasAlpha) {
			// 彩色图标
			for (size_t i = 0; i < pixelsSize; i += 4) {
				// 预乘 Alpha 通道
				double alpha = pixels[i + 3] / 255.0f;

				pixels[i] = (BYTE)std::lround(pixels[i] * alpha);
				pixels[i + 1] = (BYTE)std::lround(pixels[i + 1] * alpha);
				pixels[i + 2] = (BYTE)std::lround(pixels[i + 2] * alpha);
			}
		} else {
			// 彩色掩码图标
			for (size_t i = 0; i < pixelsSize; i += 4) {
				pixels[i + 3] = 255;
			}
		}
	}

	return bitmap;
}

static HICON GetHIconOfWnd(HWND hWnd, bool preferLargeIcon) {
	HICON result = NULL;

	if (preferLargeIcon) {
		result = (HICON)SendMessage(hWnd, WM_GETICON, ICON_BIG, 0);
		if (result) {
			return result;
		}

		result = (HICON)SendMessage(hWnd, WM_GETICON, ICON_SMALL, 0);
		if (result) {
			return result;
		}

		result = (HICON)GetClassLongPtr(hWnd, GCLP_HICON);
		if (result) {
			return result;
		}

		result = (HICON)GetClassLongPtr(hWnd, GCLP_HICONSM);
		if (result) {
			return result;
		}
	} else {
		result = (HICON)SendMessage(hWnd, WM_GETICON, ICON_SMALL, 0);
		if (result) {
			return result;
		}

		result = (HICON)SendMessage(hWnd, WM_GETICON, ICON_BIG, 0);
		if (result) {
			return result;
		}

		result = (HICON)GetClassLongPtr(hWnd, GCLP_HICONSM);
		if (result) {
			return result;
		}

		result = (HICON)GetClassLongPtr(hWnd, GCLP_HICON);
		if (result) {
			return result;
		}
	}

	// 此窗口无图标则回落到所有者窗口
	HWND hwndOwner = GetWindow(hWnd, GW_OWNER);
	if (!hwndOwner) {
		return NULL;
	}

	return GetHIconOfWnd(hwndOwner, preferLargeIcon);
}

static SoftwareBitmap GetSoftwarBitmap(HWND hWnd, bool preferLargeIcon) {
	if (HICON hIcon = GetHIconOfWnd(hWnd, preferLargeIcon)) {
		return HIcon2SoftwareBitmapAsync(hIcon);
	}

	com_ptr<IShellItemImageFactory> factory;
	HRESULT hr = SHCreateItemFromParsingName(Win32Utils::GetPathOfWnd(hWnd).c_str(), nullptr, IID_PPV_ARGS(&factory));
	if (FAILED(hr)) {
		return nullptr;
	}

	HBITMAP hBmp;
	SIZE iconSize = {
		GetSystemMetrics(preferLargeIcon ? SM_CXICON : SM_CXSMICON),
		GetSystemMetrics(preferLargeIcon ? SM_CYICON : SM_CYSMICON)
	};
	hr = factory->GetImage(iconSize, SIIGBF_BIGGERSIZEOK | SIIGBF_ICONONLY, &hBmp);
	if (FAILED(hr)) {
		return nullptr;
	}

	Utils::ScopeExit se([hBmp] {
		DeleteBitmap(hBmp);
	});

	BITMAP bmp{};
	GetObject(hBmp, sizeof(BITMAP), &bmp);

	SoftwareBitmap bitmap(BitmapPixelFormat::Bgra8, bmp.bmWidth, bmp.bmHeight, BitmapAlphaMode::Premultiplied);
	{
		BitmapBuffer buffer = bitmap.LockBuffer(BitmapBufferAccessMode::Write);
		uint8_t* pixels = buffer.CreateReference().data();

		if (!CopyPixelsOfHBmp(hBmp, bmp.bmWidth, bmp.bmHeight, pixels)) {
			return nullptr;
		}
	}
	return bitmap;
}

IAsyncOperation<ImageSource> IconHelper::GetIconOfWndAsync(HWND hWnd, bool preferLargeIcon) {
	apartment_context uiThread;
	co_await resume_background();

	SoftwareBitmap bmp = GetSoftwarBitmap(hWnd, preferLargeIcon);
	if (!bmp) {
		co_return nullptr;
	}

	co_await uiThread;
	SoftwareBitmapSource result;
	co_await result.SetBitmapAsync(bmp);
	co_return result;
}

}