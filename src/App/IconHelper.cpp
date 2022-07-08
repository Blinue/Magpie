#include "pch.h"
#include "IconHelper.h"
#include "Logger.h"
#include "Utils.h"
#include "Win32Utils.h"
#include "StrUtils.h"
#include <appmodel.h>
#include <Psapi.h>


using namespace winrt;
using namespace Windows::Graphics::Imaging;


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
		
		if (!CopyPixelsOfHBmp(ii.hbmColor, bmp.bmWidth, bmp.bmHeight, pixels)) {
			co_return nullptr;
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
	
	Imaging::SoftwareBitmapSource result;
	co_await result.SetBitmapAsync(bitmap);
	co_return result;
}

static HICON GetHIconOfWnd(HWND hWnd) {
	HICON result = (HICON)SendMessage(hWnd, WM_GETICON, ICON_SMALL, 0);
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

	// 此窗口无图标则回落到所有者窗口
	HWND hwndOwner = GetWindow(hWnd, GW_OWNER);
	if (!hwndOwner) {
		return NULL;
	}

	return GetHIconOfWnd(hwndOwner);
}

static bool IsPackaged(HWND hWnd) {
	DWORD dwProcId = 0;
	if (!GetWindowThreadProcessId(hWnd, &dwProcId)) {
		Logger::Get().Win32Error("GetWindowThreadProcessId 失败");
		return false;
	}

	Win32Utils::ScopedHandle hProc(Win32Utils::SafeHandle(OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwProcId)));
	if (!hProc) {
		Logger::Get().Win32Error("OpenProcess 失败");
		return false;
	}

	std::wstring fileName(MAX_PATH, 0);
	DWORD size = GetModuleFileNameEx(hProc.get(), NULL, fileName.data(), (DWORD)fileName.size() + 1);
	if (size == 0) {
		Logger::Get().Win32Error("GetModuleFileName 失败");
		return false;
	}
	fileName.resize(size);

	// UWP 窗口的程序名均为 ApplicationFrameHost.exe
	fileName = fileName.substr(fileName.find_last_of(L'\\') + 1);
	StrUtils::ToLowerCase(fileName);
	if (fileName == L"applicationframehost.exe") {
		return true;
	}

	// 打包应用
	UINT32 length = 0;
	LONG result = GetPackageFullName(hProc.get(), &length, nullptr);
	return result != APPMODEL_ERROR_NO_PACKAGE;
}

IAsyncOperation<ImageSource> IconHelper::GetIconOfWndAsync(HWND hWnd) {
	HICON hIcon = GetHIconOfWnd(hWnd);
	if (hIcon) {
		co_return co_await HIcon2ImageSourceAsync(hIcon);
	}

	if (IsPackaged(hWnd)) {
		co_return nullptr;
	}

	// 通过 IShellItemImageFactory 获取图标非常耗时，因此转到后台线程执行
	apartment_context uiThread;
	co_await resume_background();

	// 获取 HICON 失败则获取可执行文件图标
	com_ptr<IShellItemImageFactory> factory;
	HRESULT hr = SHCreateItemFromParsingName(Win32Utils::GetPathOfWnd(hWnd).c_str(), nullptr, IID_PPV_ARGS(&factory));
	if (FAILED(hr)) {
		co_return nullptr;
	}

	HBITMAP hBmp;
	hr = factory->GetImage({ 16,16 }, SIIGBF_BIGGERSIZEOK | SIIGBF_ICONONLY, &hBmp);
	if (FAILED(hr)) {
		co_return nullptr;
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
			co_return nullptr;
		}
	}

	co_await uiThread;
	Imaging::SoftwareBitmapSource result;
	co_await result.SetBitmapAsync(bitmap);

	co_return result;
}

}