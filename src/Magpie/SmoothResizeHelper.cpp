#include "pch.h"
#include "SmoothResizeHelper.h"
#include <inspectable.h>

// 来自 https://github.com/ALTaleX531/TranslucentFlyouts/blob/017970cbac7b77758ab6217628912a8d551fcf7c/TFModern/DiagnosticsHandler.cpp#L71-L84
DECLARE_INTERFACE_IID_(IFrameworkApplicationPrivate, IInspectable, "B3AB45D8-6A4E-4E76-A00D-32D4643A9F1A") {
	STDMETHOD(StartOnCurrentThread)(void* callback) PURE;
	STDMETHOD(CreateIsland)(void** island) PURE;
	STDMETHOD(CreateIslandWithAppWindow)(void* app_window, void** island) PURE;
	STDMETHOD(CreateIslandWithContentBridge)(void* owner, void* content_bridge, void** island) PURE;
	STDMETHOD(RemoveIsland)(void* island) PURE;
	STDMETHOD(SetSynchronizationWindow)(HWND hwnd) PURE;
};

namespace Magpie {

bool SmoothResizeHelper::EnableResizeSync(HWND hWnd, const winrt::Application& app) noexcept {
	// UWP 使用这个未记录的接口实现平滑调整尺寸
	// https://gist.github.com/apkipa/20cae438aef2a8633f99e10e0b90b11e
	static auto enableResizeLayoutSynchronization = []() {
		HMODULE hUser32 = GetModuleHandle(L"user32");
		assert(hUser32);
		using tEnableResizeLayoutSynchronization = void(WINAPI*)(HWND hwnd, BOOL enable);
		return (tEnableResizeLayoutSynchronization)GetProcAddress(hUser32, MAKEINTRESOURCEA(2615));
	}();

	// 检查是否支持 IFrameworkApplicationPrivate 接口
	if (!app.try_as<IFrameworkApplicationPrivate>() || !enableResizeLayoutSynchronization) {
		return false;
	}

	enableResizeLayoutSynchronization(hWnd, TRUE);
	return true;
}

void SmoothResizeHelper::SyncWindowSize(HWND hWnd, const winrt::Application& app) noexcept {
	// @apkipa 发现在 WM_SIZE 中调用 IFrameworkApplicationPrivate::SetSynchronizationWindow 可以防止闪烁。
	// 原理仍不清楚，似乎这个接口内部会调用 SynchronizedCommit，刚好实现了 UWP 调整大小的方法。
	app.try_as<IFrameworkApplicationPrivate>()->SetSynchronizationWindow(hWnd);
}

}
