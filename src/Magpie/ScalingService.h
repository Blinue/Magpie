#pragma once
#include <winrt/Magpie.h>
#include "Event.h"
#include "ScalingError.h"

namespace Magpie::Core {
class ScalingRuntime;
}

namespace Magpie {

struct Profile;

class ScalingService {
public:
	static ScalingService& Get() noexcept;

	ScalingService(const ScalingService&) = delete;
	ScalingService(ScalingService&&) = delete;

	~ScalingService();

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

	Core::Event<bool> IsTimerOnChanged;
	Core::Event<double> TimerTick;
	Core::Event<HWND> WndToRestoreChanged;
	Core::Event<bool> IsRunningChanged;

private:
	ScalingService() = default;

	void _WndToRestore(HWND value);

	void _ShortcutService_ShortcutPressed(winrt::Magpie::ShortcutAction action);

	void _CountDownTimer_Tick(winrt::IInspectable const&, winrt::IInspectable const&);

	winrt::fire_and_forget _CheckForegroundTimer_Tick(winrt::Threading::ThreadPoolTimer const& timer);

	void _Settings_IsAutoRestoreChanged(bool value);

	void _ScalingRuntime_IsRunningChanged(bool isRunning, ScalingError error);

	void _StartScale(HWND hWnd, const Profile& profile);

	void _ScaleForegroundWindow();

	ScalingError _CheckSrcWnd(HWND hWnd, bool checkIL) noexcept;

	std::unique_ptr<::Magpie::Core::ScalingRuntime> _scalingRuntime;
	winrt::CoreDispatcher _dispatcher{ nullptr };

	winrt::DispatcherTimer _countDownTimer;
	// DispatcherTimer 在不显示主窗口时可能停滞，因此使用 ThreadPoolTimer
	winrt::Threading::ThreadPoolTimer _checkForegroundTimer{ nullptr };

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

