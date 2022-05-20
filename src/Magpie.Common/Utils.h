#pragma once
#include <Windows.h>
#include <winrt/Windows.UI.Xaml.h>


struct Utils {
	static UINT GetOSBuild();

	static SIZE GetSizeOfRect(const RECT& rect) noexcept {
		return { rect.right - rect.left, rect.bottom - rect.top };
	}

	static void CloseAllXamlPopups(winrt::Windows::UI::Xaml::XamlRoot root);
};

inline bool operator==(const SIZE& l, const SIZE& r) {
	return l.cx == r.cx && l.cy == r.cy;
}
