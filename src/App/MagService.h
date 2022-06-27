#pragma once
#include "MagService.g.h"
#include <winrt/Magpie.Runtime.h>


namespace winrt::Magpie::App::implementation {

struct MagService : MagServiceT<MagService> {
	MagService(
		Magpie::App::Settings const& settings,
		Magpie::Runtime::MagRuntime const& magRuntime,
		Magpie::App::HotkeyManager const& hotkeyManager
	);

	void StartCountdown();

	void StopCountdown();

	bool IsCountingDown() const noexcept {
		return _tickingDownCount > 0;
	}

	event_token IsCountingDownChanged(EventHandler<bool> const& handler) {
		return _isCountingDownChangedEvent.add(handler);
	}

	void IsCountingDownChanged(event_token const& token) noexcept {
		_isCountingDownChangedEvent.remove(token);
	}

	uint32_t TickingDownCount() const noexcept {
		return _tickingDownCount;
	}

	float CountdownLeft() const noexcept;

	event_token CountdownTick(EventHandler<float> const& handler) {
		return _countdownTickEvent.add(handler);
	}

	void CountdownTick(event_token const& token) noexcept {
		_countdownTickEvent.remove(token);
	}

	uint64_t WndToRestore() const noexcept {
		return _wndToRestore;
	}

	event_token WndToRestoreChanged(EventHandler<uint64_t> const& handler) {
		return _wndToRestoreChangedEvent.add(handler);
	}

	void WndToRestoreChanged(event_token const& token) noexcept {
		_wndToRestoreChangedEvent.remove(token);
	}

	void ClearWndToRestore();

private:
	void _HotkeyManger_HotkeyPressed(IInspectable const&, HotkeyAction action);

	void _Timer_Tick(IInspectable const&, IInspectable const&);

	void _Settings_IsAutoRestoreChanged(IInspectable const&, bool);

	void _UpdateIsAutoRestore();

	void _CheckForeground();

	static void CALLBACK _WinEventProcCallback(
		HWINEVENTHOOK /*hWinEventHook*/,
		DWORD /*dwEvent*/,
		HWND /*hwnd*/,
		LONG /*idObject*/,
		LONG /*idChild*/,
		DWORD /*dwEventThread*/,
		DWORD /*dwmsEventTime*/
	);

	Magpie::App::Settings _settings{ nullptr };
	Magpie::Runtime::MagRuntime _magRuntime{ nullptr };
	CoreDispatcher _dispatcher{ nullptr };

	Magpie::App::HotkeyManager::HotkeyPressed_revoker _hotkeyPressedRevoker;
	Magpie::App::Settings::IsAutoRestoreChanged_revoker _isAutoRestoreChangedRevoker;
	Magpie::Runtime::MagRuntime::IsRunningChanged_revoker _isRunningChangedRevoker;

	DispatcherTimer _timer;
	DispatcherTimer::Tick_revoker _timerTickRevoker;
	std::chrono::steady_clock::time_point _timerStartTimePoint;

	uint32_t _tickingDownCount = 0;
	event<EventHandler<bool>> _isCountingDownChangedEvent;
	event<EventHandler<float>> _countdownTickEvent;

	HWND _curSrcWnd = 0;
	uint64_t _wndToRestore = 0;
	event<EventHandler<uint64_t>> _wndToRestoreChangedEvent;

	HWINEVENTHOOK _hForegroundEventHook = NULL;
	HWINEVENTHOOK _hDestoryEventHook = NULL;
	static MagService* _that;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct MagService : MagServiceT<MagService, implementation::MagService> {
};

}
