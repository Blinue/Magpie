#pragma once
#include "ExportHelper.h"
#include "WinRTUtils.h"
#include <Windows.h>
#include <winrt/base.h>
#include <winrt/Windows.System.h>
#include "ScalingWindow.h"

namespace Magpie::Core {

struct ScalingOptions;

class API_DECLSPEC ScalingRuntime {
public:
	ScalingRuntime();
	~ScalingRuntime();

	HWND HwndSrc() const {
		return _isRunning ? _hwndSrc : 0;
	}

	void Start(HWND hwndSrc, ScalingOptions&& options);

	void ToggleOverlay();

	void Stop();

	bool IsRunning() const noexcept {
		return _isRunning;
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

	// 确保 _dqc 完成初始化
	void _EnsureDispatcherQueue() const noexcept;

	void _IsRunning(bool value);

	HWND _hwndSrc = 0;
	std::atomic<bool> _isRunning = false;
	winrt::event<winrt::delegate<bool>> _isRunningChangedEvent;

	std::thread _scalingThread;
	winrt::Windows::System::DispatcherQueueController _dqc{ nullptr };

	// 只能由 _scalingThread 访问
	ScalingWindow _scalingWindow;
};

}
