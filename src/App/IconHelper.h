#pragma once
#include "pch.h"


namespace winrt::Magpie::App {

struct IconHelper {
	static Windows::Graphics::Imaging::SoftwareBitmap GetIconOfWnd(HWND hWnd, uint32_t preferredSize);

	static Windows::Graphics::Imaging::SoftwareBitmap GetIconOfExe(const wchar_t* path, uint32_t preferredSize);
};

}
