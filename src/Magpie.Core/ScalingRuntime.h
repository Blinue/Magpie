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

	void Start(HWND hwndSrc, struct ScalingOptions&& options);

	void ToggleOverlay();

	void Stop();

	bool IsRunning() const noexcept {
		return _state.load(std::memory_order_relaxed) != _State::Idle;
	}

	// 调用者应处理线程同步
	WinRTUtils::Event<winrt::delegate<bool>> IsRunningChanged;

private:
	void _ScalingThreadProc() noexcept;

	// 确保 _dispatcher 完成初始化
	const winrt::DispatcherQueue& _Dispatcher() noexcept;

	enum class _State {
		Idle,
		Initializing,
		Scaling
	};
	std::atomic<_State> _state{ _State::Idle };

	winrt::DispatcherQueue _dispatcher{ nullptr };
	std::atomic<bool> _dispatcherInitialized = false;
	// 只能在主线程访问，省下检查 _dispatcherInitialized 的开销
	bool _dispatcherInitializedCache = false;
	// 应在 _dispatcher 后初始化
	std::thread _scalingThread;
};

}
