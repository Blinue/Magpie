#pragma once
#include "WinRTUtils.h"
#include <Windows.h>
#include <winrt/base.h>
#include <winrt/Windows.System.h>

namespace Magpie::Core {

class ScalingRuntime {
public:
	ScalingRuntime();
	~ScalingRuntime();

	HWND HwndSrc() const {
		return _hwndSrc.load(std::memory_order_relaxed);
	}

	void Start(HWND hwndSrc, struct ScalingOptions&& options);

	void ToggleOverlay();

	void Stop();

	bool IsRunning() const noexcept {
		return HwndSrc();
	}

	// 调用者应处理线程同步
	winrt::event_token IsRunningChanged(winrt::delegate<bool> const& handler) {
		return _isRunningChangedEvent.add(handler);
	}

	WinRTUtils::EventRevoker IsRunningChanged(winrt::auto_revoke_t, winrt::delegate<bool> const& handler) {
		winrt::event_token token = IsRunningChanged(handler);
		return WinRTUtils::EventRevoker([this, token]() {
			IsRunningChanged(token);
		});
	}

	void IsRunningChanged(winrt::event_token const& token) noexcept {
		_isRunningChangedEvent.remove(token);
	}

private:
	void _ScalingThreadProc() noexcept;

	// 确保 _dispatcher 完成初始化
	const winrt::DispatcherQueue& _Dispatcher() noexcept;

	// 主线程使用 DispatcherQueue 和缩放线程沟通，因此无需约束内存定序，只需确保原子性即可
	std::atomic<HWND> _hwndSrc;
	winrt::event<winrt::delegate<bool>> _isRunningChangedEvent;

	winrt::DispatcherQueue _dispatcher{ nullptr };
	std::atomic<bool> _dispatcherInitialized = false;
	// 只能在主线程访问，省下检查 _dispatcherInitialized 的开销
	bool _dispatcherInitializedCache = false;
	// 应在 _dispatcher 后初始化
	std::thread _scalingThread;
};

}
