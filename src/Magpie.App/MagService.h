#pragma once
#include <winrt/Magpie.App.h>
#include <Magpie.Core.h>
#include "WinRTUtils.h"

namespace winrt::Magpie::App {

struct ScalingProfile;

class MagService {
public:
	static MagService& Get() noexcept {
		static MagService instance;
		return instance;
	}

	MagService(const MagService&) = delete;
	MagService(MagService&&) = delete;

	void Initialize();

	void StartCountdown();

	void StopCountdown();

	bool IsCountingDown() const noexcept {
		return _tickingDownCount > 0;
	}

	event_token IsCountingDownChanged(delegate<bool> const& handler) {
		return _isCountingDownChangedEvent.add(handler);
	}

	WinRTUtils::EventRevoker IsCountingDownChanged(auto_revoke_t, delegate<bool> const& handler) {
		event_token token = IsCountingDownChanged(handler);
		return WinRTUtils::EventRevoker([this, token]() {
			IsCountingDownChanged(token);
		});
	}

	void IsCountingDownChanged(event_token const& token) noexcept {
		_isCountingDownChangedEvent.remove(token);
	}

	uint32_t TickingDownCount() const noexcept {
		return _tickingDownCount;
	}

	float CountdownLeft() const noexcept;

	event_token CountdownTick(delegate<float> const& handler) {
		return _countdownTickEvent.add(handler);
	}

	WinRTUtils::EventRevoker CountdownTick(auto_revoke_t, delegate<float> const& handler) {
		event_token token = CountdownTick(handler);
		return WinRTUtils::EventRevoker([this, token]() {
			CountdownTick(token);
		});
	}

	void CountdownTick(event_token const& token) noexcept {
		_countdownTickEvent.remove(token);
	}

	HWND WndToRestore() const noexcept {
		return _hwndToRestore;
	}

	event_token WndToRestoreChanged(delegate<HWND> const& handler) {
		return _wndToRestoreChangedEvent.add(handler);
	}

	WinRTUtils::EventRevoker WndToRestoreChanged(auto_revoke_t, delegate<HWND> const& handler) {
		event_token token = WndToRestoreChanged(handler);
		return WinRTUtils::EventRevoker([this, token]() {
			WndToRestoreChanged(token);
		});
	}

	void WndToRestoreChanged(event_token const& token) noexcept {
		_wndToRestoreChangedEvent.remove(token);
	}

	void ClearWndToRestore();

	event_token IsRunningChanged(delegate<bool> const& handler) {
		return _isRunningChangedEvent.add(handler);
	}

	WinRTUtils::EventRevoker IsRunningChanged(auto_revoke_t, delegate<bool> const& handler) {
		event_token token = IsRunningChanged(handler);
		return WinRTUtils::EventRevoker([this, token]() {
			IsRunningChanged(token);
		});
	}

	void IsRunningChanged(event_token const& token) noexcept {
		_isRunningChangedEvent.remove(token);
	}

	bool IsRunning() const noexcept {
		return _magRuntime.IsRunning();
	}

private:
	MagService() = default;

	void _WndToRestore(HWND value);

	void _HotkeyService_HotkeyPressed(HotkeyAction action);

	void _CountDownTimer_Tick(IInspectable const&, IInspectable const&);

	void _CheckForegroundTimer_Tick(IInspectable const&, IInspectable const&);

	void _Settings_IsAutoRestoreChanged(bool);

	fire_and_forget _MagRuntime_IsRunningChanged(bool isRunning);

	void _StartScale(HWND hWnd, const ScalingProfile& profile);

	void _ScaleForegroundWindow();

	bool _CheckSrcWnd(HWND hWnd) noexcept;

	::Magpie::Core::MagRuntime _magRuntime;
	CoreDispatcher _dispatcher{ nullptr };

	DispatcherTimer _countDownTimer;
	DispatcherTimer _checkForegroundtimer;

	std::chrono::steady_clock::time_point _timerStartTimePoint;

	uint32_t _tickingDownCount = 0;

	HWND _hwndCurSrc = NULL;
	HWND _hwndToRestore = NULL;
	// 放大后用户使用热键退出全屏后暂时阻止该窗口自动放大
	HWND _hwndtTempException = NULL;

	event<delegate<bool>> _isCountingDownChangedEvent;
	event<delegate<float>> _countdownTickEvent;
	event<delegate<HWND>> _wndToRestoreChangedEvent;
	event<delegate<bool>> _isRunningChangedEvent;
	
	bool _isAutoScaling = false;
};

}

