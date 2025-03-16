#pragma once

namespace Magpie {

struct SmoothResizeHelper {
	// 初始化 XAML Islands 后调用
	static bool EnableResizeSync(HWND hWnd, const winrt::Application& app) noexcept;

	// WM_SIZE 中调用
	static void SyncWindowSize(HWND hWnd, const winrt::Application& app) noexcept;
};

}
