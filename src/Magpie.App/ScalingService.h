#pragma once
#include <winrt/Magpie.App.h>
#include <Magpie.Core.h>
#include "WinRTUtils.h"

namespace winrt::Magpie::App {

struct Profile;

class ScalingService {
public:
	static ScalingService& Get() noexcept {
		static ScalingService instance;
		return instance;
	}

	ScalingService(const ScalingService&) = delete;
	ScalingService(ScalingService&&) = delete;

	void Initialize();

	void Uninitialize();

	void StartTimer();

	void StopTimer();

	bool IsTimerOn() const noexcept {
		return _curCountdownSeconds > 0;
	}

	event_token IsTimerOnChanged(delegate<bool> const& handler) {
		return _isTimerOnChangedEvent.add(handler);
	}

	WinRTUtils::EventRevoker IsTimerOnChanged(auto_revoke_t, delegate<bool> const& handler) {
		event_token token = IsTimerOnChanged(handler);
		return WinRTUtils::EventRevoker([this, token]() {
			IsTimerOnChanged(token);
		});
	}

	void IsTimerOnChanged(event_token const& token) noexcept {
		_isTimerOnChangedEvent.remove(token);
	}

	double TimerProgress() const noexcept {
		return SecondsLeft() / _curCountdownSeconds;
	}

	double SecondsLeft() const noexcept;

	event_token TimerTick(delegate<double> const& handler) {
		return _timerTickEvent.add(handler);
	}

	WinRTUtils::EventRevoker TimerTick(auto_revoke_t, delegate<double> const& handler) {
		event_token token = TimerTick(handler);
		return WinRTUtils::EventRevoker([this, token]() {
			TimerTick(token);
		});
	}

	void TimerTick(event_token const& token) noexcept {
		_timerTickEvent.remove(token);
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
		return _ScalingRuntime->IsRunning();
	}

	// 强制重新检查前台窗口
	void CheckForeground();

private:
	ScalingService() = default;

	void _WndToRestore(HWND value);

	void _ShortcutService_ShortcutPressed(ShortcutAction action);

	void _CountDownTimer_Tick(IInspectable const&, IInspectable const&);

	fire_and_forget _CheckForegroundTimer_Tick(Threading::ThreadPoolTimer const& timer);

	void _Settings_IsAutoRestoreChanged(bool);

	fire_and_forget _ScalingRuntime_IsRunningChanged(bool isRunning);

	bool _StartScale(HWND hWnd, const Profile& profile);

	void _ScaleForegroundWindow();

	bool _CheckSrcWnd(HWND hWnd) noexcept;

	std::optional<::Magpie::Core::ScalingRuntime> _ScalingRuntime;
	CoreDispatcher _dispatcher{ nullptr };

	DispatcherTimer _countDownTimer;
	// DispatcherTimer 在不显示主窗口时可能停滞，因此使用 ThreadPoolTimer
	Threading::ThreadPoolTimer _checkForegroundTimer{ nullptr };

	std::chrono::steady_clock::time_point _timerStartTimePoint;

	uint32_t _curCountdownSeconds = 0;

	HWND _hwndCurSrc = NULL;
	HWND _hwndToRestore = NULL;
	// 1. 避免重复检查同一个窗口
	// 2. 用户使用热键退出全屏后暂时阻止该窗口自动放大
	// 可能在线程池中访问，因此增加原子性
	std::atomic<HWND> _hwndChecked = NULL;

	event<delegate<bool>> _isTimerOnChangedEvent;
	event<delegate<double>> _timerTickEvent;
	event<delegate<HWND>> _wndToRestoreChangedEvent;
	event<delegate<bool>> _isRunningChangedEvent;
	
	bool _isAutoScaling = false;
};

}

