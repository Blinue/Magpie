#pragma once

namespace Magpie {

struct ThemeHelper {
	// 应用程序启动时调用一次
	static void Initialize() noexcept;
	static void SetWindowTheme(HWND hWnd, bool darkBorder, bool darkMenu) noexcept;

	static constexpr COLORREF LIGHT_TINT_COLOR = RGB(243, 243, 243);
	static constexpr COLORREF DARK_TINT_COLOR = RGB(32, 32, 32);
};

}
