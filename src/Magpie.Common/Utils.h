#pragma once
#include <Windows.h>


struct Utils {
	static UINT GetOSBuild();

	static SIZE GetSizeOfRect(const RECT& rect) noexcept {
		return { rect.right - rect.left, rect.bottom - rect.top };
	}
};

inline bool operator==(const SIZE& l, const SIZE& r) {
	return l.cx == r.cx && l.cy == r.cy;
}
