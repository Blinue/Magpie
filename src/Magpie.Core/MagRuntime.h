#pragma once
#include "ExportHelper.h"
#include "WinRTUtils.h"
#include <Windows.h>
#include <winrt/base.h>
#include <winrt/Windows.System.h>

namespace Magpie::Core {

struct MagOptions;

class API_DECLSPEC MagRuntime {
public:
	MagRuntime();
	~MagRuntime();

	HWND HwndSrc() const {
		return _running ? _hwndSrc : 0;
	}

	void Run(HWND hwndSrc, const MagOptions& options);

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

	std::thread _magWindThread;
	std::atomic<bool> _running = false;
	HWND _hwndSrc = 0;
	winrt::Windows::System::DispatcherQueueController _dqc{ nullptr };

	winrt::event<winrt::delegate<bool>> _isRunningChangedEvent;
};

}
