#pragma once
#include <winrt/Magpie.App.h>
#include "WinRTUtils.h"
#include "ScalingError.h"

namespace Magpie::Core {
class ScalingRuntime;
}

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

	double TimerProgress() const noexcept {
		return SecondsLeft() / _curCountdownSeconds;
	}

	double SecondsLeft() const noexcept;

	HWND WndToRestore() const noexcept {
		return _hwndToRestore;
	}

	void ClearWndToRestore();

	bool IsRunning() const noexcept;

	// 强制重新检查前台窗口
	void CheckForeground();

	WinRTUtils::Event<delegate<bool>> IsTimerOnChanged;
	WinRTUtils::Event<delegate<double>> TimerTick;
	WinRTUtils::Event<delegate<HWND>> WndToRestoreChanged;
	WinRTUtils::Event<delegate<bool>> IsRunningChanged;

private:
	ScalingService() = default;

	void _WndToRestore(HWND value);

	void _ShortcutService_ShortcutPressed(ShortcutAction action);

	void _CountDownTimer_Tick(IInspectable const&, IInspectable const&);

	fire_and_forget _CheckForegroundTimer_Tick(Threading::ThreadPoolTimer const& timer);

	void _Settings_IsAutoRestoreChanged(bool value);

	fire_and_forget _ScalingRuntime_IsRunningChanged(bool isRunning);

	void _StartScale(HWND hWnd, const Profile& profile);

	void _ScaleForegroundWindow();

	bool _CheckSrcWnd(HWND hWnd, bool checkIL) noexcept;

	std::unique_ptr<::Magpie::Core::ScalingRuntime> _scalingRuntime;
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
	
	bool _isAutoScaling = false;
};

}

