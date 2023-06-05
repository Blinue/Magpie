#pragma once
#include "ExportHelper.h"
#include "WinRTUtils.h"
#include <Windows.h>
#include <winrt/base.h>
#include <winrt/Windows.System.h>

namespace Magpie::Core {

struct ScalingOptions;

class API_DECLSPEC ScalingRuntime {
public:
	ScalingRuntime();
	~ScalingRuntime();

	HWND HwndSrc() const {
		return _running ? _hwndSrc : 0;
	}

	void Start(HWND hwndSrc, const ScalingOptions& options);

	void ToggleOverlay();

	void Stop();

	bool IsRunning() const {
		return _running;
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

	std::thread _scalingWndThread;
	std::atomic<bool> _running = false;
	HWND _hwndSrc = 0;
	winrt::Windows::System::DispatcherQueueController _dqc{ nullptr };

	winrt::event<winrt::delegate<bool>> _isRunningChangedEvent;
};

}
