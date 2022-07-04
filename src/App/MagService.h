#pragma once
#include <winrt/Magpie.App.h>
#include <winrt/Magpie.Runtime.h>
#include "WinRTUtils.h"


namespace winrt::Magpie::App {

class MagService {
public:
	static MagService& Get() {
		static MagService instance;
		return instance;
	}

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

	uint64_t WndToRestore() const noexcept {
		return _wndToRestore;
	}

	event_token WndToRestoreChanged(delegate<uint64_t> const& handler) {
		return _wndToRestoreChangedEvent.add(handler);
	}

	WinRTUtils::EventRevoker WndToRestoreChanged(auto_revoke_t, delegate<uint64_t> const& handler) {
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
	MagService();

	MagService(const MagService&) = delete;
	MagService(MagService&&) = delete;

	void _HotkeyService_HotkeyPressed(HotkeyAction action);

	void _Timer_Tick(IInspectable const&, IInspectable const&);

	void _Settings_IsAutoRestoreChanged(bool);

	IAsyncAction _MagRuntime_IsRunningChanged(IInspectable const&, bool);

	void _UpdateIsAutoRestore();

	void _CheckForeground();

	void _StartScale(uint64_t hWnd = 0);

	static void CALLBACK _WinEventProcCallback(
		HWINEVENTHOOK /*hWinEventHook*/,
		DWORD /*dwEvent*/,
		HWND /*hwnd*/,
		LONG /*idObject*/,
		LONG /*idChild*/,
		DWORD /*dwEventThread*/,
		DWORD /*dwmsEventTime*/
	);

	Magpie::Runtime::MagRuntime _magRuntime;
	CoreDispatcher _dispatcher{ nullptr };

	DispatcherTimer _timer;
	DispatcherTimer::Tick_revoker _timerTickRevoker;
	std::chrono::steady_clock::time_point _timerStartTimePoint;

	uint32_t _tickingDownCount = 0;
	event<delegate<bool>> _isCountingDownChangedEvent;
	event<delegate<float>> _countdownTickEvent;

	HWND _curSrcWnd = 0;
	uint64_t _wndToRestore = 0;
	event<delegate<uint64_t>> _wndToRestoreChangedEvent;

	event<delegate<bool>> _isRunningChangedEvent;

	HWND _hwndHost = 0;

	HWINEVENTHOOK _hForegroundEventHook = NULL;
	HWINEVENTHOOK _hDestoryEventHook = NULL;
};

}

