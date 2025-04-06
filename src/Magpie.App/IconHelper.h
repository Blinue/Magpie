#pragma once

namespace winrt::Magpie::App {

struct IconHelper {
	static Windows::Graphics::Imaging::SoftwareBitmap ExtractIconFormWnd(HWND hWnd, uint32_t preferredSize, uint32_t dpi);
	static Windows::Graphics::Imaging::SoftwareBitmap ExtractIconFromExe(const wchar_t* fileName, uint32_t preferredSize, uint32_t dpi);
	static Windows::Graphics::Imaging::SoftwareBitmap ExtractAppIcon(uint32_t preferredSize);
};

}
