#pragma once

namespace Magpie {

struct IconHelper {
	static winrt::Windows::Graphics::Imaging::SoftwareBitmap ExtractIconFormWnd(HWND hWnd, uint32_t preferredSize);
	static winrt::Windows::Graphics::Imaging::SoftwareBitmap ExtractIconFromExe(const wchar_t* fileName, uint32_t preferredSize);
	static winrt::Windows::Graphics::Imaging::SoftwareBitmap ExtractAppSmallIcon();
	static winrt::Windows::Graphics::Imaging::SoftwareBitmap ExtractAppIcon(uint32_t preferredSize);
};

}
