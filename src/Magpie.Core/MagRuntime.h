#pragma once
#include "WinRTUtils.h"
#include <Windows.h>
#include <winrt/base.h>
#include <winrt/Windows.System.h>

namespace Magpie::Core {

struct MagOptions;

class MagRuntime {
public:
	MagRuntime();
	~MagRuntime();

	HWND HwndSrc() const {
		return _hwndSrc.load(std::memory_order_relaxed);
	}

	void Run(HWND hwndSrc, const MagOptions& options);

	void ToggleOverlay();

	void Stop();

	bool IsRunning() const {
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
	void _MagWindThreadProc() noexcept;

	// 确保 _dqc 完成初始化
	void _EnsureDispatcherQueue() const noexcept;

	// 主线程使用 DispatcherQueue 和缩放线程沟通，因此无需约束内存定序，只需确保原子性即可
	std::atomic<HWND> _hwndSrc;
	winrt::event<winrt::delegate<bool>> _isRunningChangedEvent;

	winrt::Windows::System::DispatcherQueueController _dqc{ nullptr };
	// 应在 _dqc 后初始化
	std::thread _magWindThread;
};

}
