#include "pch.h"
#include "IconHelper.h"
#include "Logger.h"
#include "Utils.h"
#include "Win32Utils.h"
#include "StrUtils.h"

using namespace winrt;
using namespace Windows::Graphics::Imaging;
using namespace Windows::UI::Xaml::Media::Imaging;


namespace winrt::Magpie::App {

static bool CopyPixelsOfHBmp(HBITMAP hBmp, LONG width, LONG height, void* data) noexcept {
	BITMAPINFO bi = {
		.bmiHeader = {
			.biSize = sizeof(BITMAPINFOHEADER),
			.biWidth = width,
			.biHeight = -height,
			.biPlanes = 1,
			.biBitCount = 32,
			.biCompression = BI_RGB,
			.biSizeImage = DWORD(width * height * 4)
		}
	};

	wil::unique_hdc_window hdc(wil::window_dc(GetDC(NULL)));
	if (GetDIBits(hdc.get(), hBmp, 0, height, data, &bi, DIB_RGB_COLORS) != height) {
		Logger::Get().Win32Error("GetDIBits 失败");
		return false;
	}

	return true;
}

static SoftwareBitmap HIcon2SoftwareBitmap(HICON hIcon) {
	// 支持彩色光标和彩色掩码图标，不支持单色图标

	ICONINFO iconInfo{};
	if (!GetIconInfo(hIcon, &iconInfo)) {
		Logger::Get().Win32Error("GetIconInfo 失败");
		return nullptr;
	}

	wil::unique_hbitmap hbmpColor(iconInfo.hbmColor);
	wil::unique_hbitmap hbmpMask(iconInfo.hbmMask);

	if (!iconInfo.fIcon) {
		return nullptr;
	}

	BITMAP bmp{};
	GetObject(iconInfo.hbmColor, sizeof(BITMAP), &bmp);

	SoftwareBitmap bitmap(BitmapPixelFormat::Bgra8, bmp.bmWidth, bmp.bmHeight, BitmapAlphaMode::Premultiplied);
	{
		BitmapBuffer buffer = bitmap.LockBuffer(BitmapBufferAccessMode::Write);
		uint8_t* pixels = buffer.CreateReference().data();

		if (!CopyPixelsOfHBmp(iconInfo.hbmColor, bmp.bmWidth, bmp.bmHeight, pixels)) {
			return nullptr;
		}

		const uint32_t pixelsSize = bmp.bmWidth * bmp.bmHeight * 4;

		// 若颜色掩码有 A 通道，则是彩色图标，否则是彩色掩码图标
		bool hasAlpha = false;
		for (uint32_t i = 3; i < pixelsSize; i += 4) {
			if (pixels[i] != 0) {
				hasAlpha = true;
				break;
			}
		}

		if (hasAlpha) {
			// 彩色图标
			for (size_t i = 0; i < pixelsSize; i += 4) {
				// 预乘 Alpha 通道
				const float alpha = pixels[i + 3] / 255.0f;

				pixels[i] = (uint8_t)std::lroundf(pixels[i] * alpha);
				pixels[i + 1] = (uint8_t)std::lroundf(pixels[i + 1] * alpha);
				pixels[i + 2] = (uint8_t)std::lroundf(pixels[i + 2] * alpha);
			}
		} else if (iconInfo.hbmMask) {
			// 彩色掩码图标
			std::unique_ptr<uint8_t[]> maskData = std::make_unique<uint8_t[]>(pixelsSize);
			if (!CopyPixelsOfHBmp(iconInfo.hbmMask, bmp.bmWidth, bmp.bmHeight, maskData.get())) {
				return nullptr;
			}

			for (uint32_t i = 0; i < pixelsSize; i += 4) {
				// hbmMask 表示是否应用掩码
				// 如果需要应用掩码而掩码不为零，那么这个图标无法转换为彩色图标，这种情况下直接忽略掩码
				if (maskData[i] != 0 && pixels[i] == 0 && pixels[i + 1] == 0 && pixels[i + 2] == 0) {
					// 掩码全为 0 表示透明像素
					std::memset(pixels + i, 0, 4);
				} else {
					pixels[i + 3] = 255;
				}
			}
		} else {
			for (uint32_t i = 3; i < pixelsSize; i += 4) {
				pixels[i] = 255;
			}
		}
	}

	return bitmap;
}

static SIZE GetSizeOfIcon(HICON hIcon) noexcept {
	ICONINFO iconInfo{};
	if (!GetIconInfo(hIcon, &iconInfo)) {
		Logger::Get().Win32Error("GetIconInfo 失败");
		return {};
	}

	wil::unique_hbitmap hbmpColor(iconInfo.hbmColor);
	wil::unique_hbitmap hbmpMask(iconInfo.hbmMask);

	if (!iconInfo.fIcon) {
		return {};
	}

	BITMAP bmp{};
	GetObject(iconInfo.hbmColor, sizeof(BITMAP), &bmp);
	return { bmp.bmWidth, bmp.bmHeight };
}

static HICON GetHIconOfWnd(HWND hWnd, LONG preferredSize) noexcept {
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

SoftwareBitmap IconHelper::ExtractIconFormWnd(HWND hWnd, uint32_t preferredSize, uint32_t dpi) {
	if (HICON hIcon = GetHIconOfWnd(hWnd, std::lround(preferredSize * dpi / double(USER_DEFAULT_SCREEN_DPI)))) {
		return HIcon2SoftwareBitmap(hIcon);
	}

	return ExtractIconFromExe(Win32Utils::GetPathOfWnd(hWnd).c_str(), preferredSize, dpi);
}

SoftwareBitmap IconHelper::ExtractIconFromExe(const wchar_t* fileName, uint32_t preferredSize, uint32_t dpi) {
	preferredSize = (uint32_t)std::lround(preferredSize * dpi / double(USER_DEFAULT_SCREEN_DPI));

	{
		wil::unique_hicon hIcon = NULL;
		SHDefExtractIcon(fileName, 0, 0, hIcon.put(), NULL, preferredSize);
		if (hIcon) {
			return HIcon2SoftwareBitmap(hIcon.get());
		}
	}

	// 回落到 IShellItemImageFactory，此接口存在以下问题:
	// 1. 速度较慢
	// 2. SIIGBF_BIGGERSIZEOK 不起作用，在我的测试里它始终在内部执行低质量的 GDI 缩放
	// 3. 不能可靠的并发使用，有时会得到错误的结果
	// 4. 据说它返回的位图有时已经预乘透明通道，没有区分的办法

	wil::unique_hbitmap hBmp;
	{
		static wil::srwlock srwLock;
		auto lock = srwLock.lock_exclusive();

		com_ptr<IShellItemImageFactory> factory;
		HRESULT hr = SHCreateItemFromParsingName(fileName, nullptr, IID_PPV_ARGS(&factory));
		if (FAILED(hr)) {
			return nullptr;
		}

		while (true) {
			hr = factory->GetImage(
				{ (LONG)preferredSize, (LONG)preferredSize },
				SIIGBF_BIGGERSIZEOK | SIIGBF_ICONONLY,
				hBmp.put()
			);

			if (hr == E_PENDING) {
				// 等待提取完成
				Sleep(0);
				continue;
			} else if (FAILED(hr)) {
				return nullptr;
			} else {
				break;
			}
		}
	}

	BITMAP bmp{};
	GetObject(hBmp.get(), sizeof(BITMAP), &bmp);

	SoftwareBitmap bitmap(BitmapPixelFormat::Bgra8, bmp.bmWidth, bmp.bmHeight, BitmapAlphaMode::Premultiplied);
	{
		BitmapBuffer buffer = bitmap.LockBuffer(BitmapBufferAccessMode::Write);
		uint8_t* pixels = buffer.CreateReference().data();

		if (!CopyPixelsOfHBmp(hBmp.get(), bmp.bmWidth, bmp.bmHeight, pixels)) {
			return nullptr;
		}

		const size_t pixelsSize = size_t(bmp.bmWidth * bmp.bmHeight * 4);
		for (size_t i = 0; i < pixelsSize; i += 4) {
			// 预乘 Alpha 通道
			float alpha = pixels[i + 3] / 255.0f;

			pixels[i] = (BYTE)std::lroundf(pixels[i] * alpha);
			pixels[i + 1] = (BYTE)std::lroundf(pixels[i + 1] * alpha);
			pixels[i + 2] = (BYTE)std::lroundf(pixels[i + 2] * alpha);
		}
	}

	return bitmap;
}

}
