#pragma once
#include "pch.h"


namespace winrt::Magpie::UI {

struct IconHelper {
	static Windows::Graphics::Imaging::SoftwareBitmap GetIconOfWnd(HWND hWnd, uint32_t preferredSize, uint32_t dpi);

	static Windows::Graphics::Imaging::SoftwareBitmap GetIconOfExe(const wchar_t* path, uint32_t preferredSize, uint32_t dpi);
};

}
