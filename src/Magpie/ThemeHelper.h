#pragma once

namespace Magpie {

struct ThemeHelper {
	static void Initialize() noexcept;
	static void SetTheme(HWND hWnd, bool isDark) noexcept;
};

}
