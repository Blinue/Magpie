#pragma once

namespace Magpie {

struct ThemeHelper {
	// 应用程序启动时调用一次
	static void Initialize() noexcept;
	static void SetWindowTheme(HWND hWnd, bool darkBorder, bool darkMenu) noexcept;
};

}
