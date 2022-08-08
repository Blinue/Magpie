#pragma once
#include "CommonPCH.h"
#include "MagOptions.h"
#include "WinRTUtils.h"


namespace Magpie::Runtime {

class API_DECLSPEC MagRuntime {
public:
	MagRuntime();
	~MagRuntime();

	HWND HwndSrc() const;

	void Run(HWND hwndSrc, MagOptions& options);

	void ToggleOverlay();

	void Stop();

	bool IsRunning() const;

	// 调用者应处理线程同步
	winrt::event_token IsRunningChanged(winrt::delegate<bool> const& handler);

	WinRTUtils::EventRevoker IsRunningChanged(winrt::auto_revoke_t, winrt::delegate<bool> const& handler) {
		winrt::event_token token = IsRunningChanged(handler);
		return WinRTUtils::EventRevoker([this, token]() {
			IsRunningChanged(token);
		});
	}

	void IsRunningChanged(winrt::event_token const& token) noexcept;

private:
	class Impl;
	Impl* _impl;
};

}
