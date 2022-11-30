#pragma once
#include "pch.h"

namespace Magpie::Core {

struct WindowHelper {
	static bool IsStartMenu(HWND hWnd) noexcept;

	static bool IsValidSrcWindow(HWND hwndSrc) noexcept;
};

}
