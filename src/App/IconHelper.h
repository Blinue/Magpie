#pragma once
#include "pch.h"


namespace winrt::Magpie::App {

struct IconHelper {
	static IAsyncOperation<ImageSource> HIcon2ImageSourceAsync(HICON hIcon);

	static IAsyncOperation<ImageSource> GetIconOfWndAsync(HWND hWnd);
};

}
