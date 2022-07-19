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
				float alpha = pixels[i + 3] / 255.0f;

				pixels[i] = (BYTE)std::lroundf(pixels[i] * alpha);
				pixels[i + 1] = (BYTE)std::lroundf(pixels[i + 1] * alpha);
				pixels[i + 2] = (BYTE)std::lroundf(pixels[i + 2] * alpha);
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

static SIZE GetSizeOfIcon(HICON hIcon) {
	ICONINFO ii{};
	if (!GetIconInfo(hIcon, &ii)) {
		Logger::Get().Win32Error("GetIconInfo 失败");
		return {};
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
		return {};
	}

	BITMAP bmp{};
	GetObject(ii.hbmColor, sizeof(BITMAP), &bmp);
	return { bmp.bmWidth, bmp.bmHeight };
}

static HICON GetHIconOfWnd(HWND hWnd, LONG preferredSize) {
	HICON result = NULL;

	HICON candidateSmallIcon = NULL;

	result = (HICON)SendMessage(hWnd, WM_GETICON, ICON_SMALL, 0);
	if (result) {
		if (GetSizeOfIcon(result).cx >= preferredSize) {
			// 小图标尺寸足够
			return result;
		} else {
			// 否则继续检索大图标
			candidateSmallIcon = result;
		}
	}

	result = (HICON)SendMessage(hWnd, WM_GETICON, ICON_BIG, 0);
	if (result) {
		return result;
	}

	if (!candidateSmallIcon) {
		result = (HICON)GetClassLongPtr(hWnd, GCLP_HICONSM);
		if (result) {
			if (GetSizeOfIcon(result).cx >= preferredSize) {
				return result;
			} else {
				candidateSmallIcon = result;
			}
		}
	}

	result = (HICON)GetClassLongPtr(hWnd, GCLP_HICON);
	if (result) {
		return result;
	}

	if (candidateSmallIcon) {
		// 不存在大图标则回落到小图标
		return candidateSmallIcon;
	}

	// 此窗口无图标则回落到所有者窗口
	HWND hwndOwner = GetWindow(hWnd, GW_OWNER);
	if (!hwndOwner) {
		return NULL;
	}

	return GetHIconOfWnd(hwndOwner, preferredSize);
}

SoftwareBitmap IconHelper::GetIconOfWnd(HWND hWnd, uint32_t preferredSize) {
	if (HICON hIcon = GetHIconOfWnd(hWnd, (LONG)preferredSize)) {
		return HIcon2SoftwareBitmapAsync(hIcon);
	}

	return GetIconOfExe(Win32Utils::GetPathOfWnd(hWnd).c_str(), preferredSize);
}

SoftwareBitmap IconHelper::GetIconOfExe(const wchar_t* path, uint32_t preferredSize) {
	static Win32Utils::SRWMutex mutex;

	HBITMAP hBmp = NULL;
	{
		// 并发使用 IShellItemImageFactory 有时会得到错误的结果
		std::scoped_lock lk(mutex);

		com_ptr<IShellItemImageFactory> factory;
		HRESULT hr = SHCreateItemFromParsingName(path, nullptr, IID_PPV_ARGS(&factory));
		if (FAILED(hr)) {
			return nullptr;
		}

		while (true) {
			hr = factory->GetImage({ (LONG)preferredSize, (LONG)preferredSize }, SIIGBF_BIGGERSIZEOK | SIIGBF_ICONONLY, &hBmp);

			if (hr == E_PENDING) {
				Sleep(0);
				continue;
			} else if (FAILED(hr)) {
				return nullptr;
			} else {
				break;
			}
		}
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

}
