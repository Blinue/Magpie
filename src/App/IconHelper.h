#pragma once
#include "pch.h"


namespace winrt::Magpie::App {

struct IconHelper {
	static IAsyncOperation<ImageSource> GetIconOfWndAsync(HWND hWnd, bool preferLargeIcon);
};

}
