#pragma once
#include "pch.h"


namespace winrt::Magpie::App {

struct IconHelper {
	static IAsyncOperation<Windows::Graphics::Imaging::SoftwareBitmap> GetIconOfWndAsync(HWND hWnd, SIZE preferredSize);
};

}
