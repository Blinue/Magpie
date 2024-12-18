#pragma once

namespace Magpie {

struct WindowHelper {
	static bool IsStartMenu(HWND hWnd) noexcept;

	static bool IsForbiddenSystemWindow(HWND hwndSrc) noexcept;
};

}
