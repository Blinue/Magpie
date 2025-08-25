#pragma once

struct Utils {
	static RECT MonitorRectFromWindow(HWND hWnd) noexcept;

	static HINSTANCE GetModuleInstanceHandle() noexcept;
};
