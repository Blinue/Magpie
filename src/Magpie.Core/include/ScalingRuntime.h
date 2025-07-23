#pragma once
#include "Event.h"

namespace Magpie {

class ScalingRuntime {
public:
	ScalingRuntime();
	~ScalingRuntime();

	bool Start(HWND hwndSrc, struct ScalingOptions&& options);

	void SwitchScalingState(bool isWindowedMode);

	void SwitchToolbarState();

	void Stop();

	bool IsScaling() const noexcept {
		return _isScaling.load(std::memory_order_relaxed);
	}

	// 调用者应处理线程同步
	MultithreadEvent<bool> IsScalingChanged;

private:
	void _ScalingThreadProc() noexcept;

	// 确保 _dispatcher 完成初始化
	const winrt::DispatcherQueue& _Dispatcher() noexcept;

	void _IsScaling(bool value);

	std::thread _scalingThread;

	winrt::DispatcherQueue _dispatcher{ nullptr };
	std::atomic<bool> _dispatcherInitialized = false;
	// 只能在主线程访问，省下检查 _dispatcherInitialized 的开销
	bool _dispatcherInitializedCache = false;

	std::atomic<bool> _isScaling = false;
};

}
