#pragma once
#include <winrt/Magpie.h>
#include <winrt/Windows.System.Threading.h>
#include "Event.h"
#include "ScalingError.h"

namespace Magpie {
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

	bool IsRunning() const noexcept;

	// 强制重新检查前台窗口
	void CheckForeground();

	Event<bool> IsTimerOnChanged;
	Event<double> TimerTick;
	Event<bool> IsRunningChanged;

private:
	ScalingService() = default;

	void _ShortcutService_ShortcutPressed(winrt::Magpie::ShortcutAction action);

	void _CountDownTimer_Tick(winrt::IInspectable const&, winrt::IInspectable const&);

	winrt::fire_and_forget _CheckForegroundTimer_Tick(winrt::Threading::ThreadPoolTimer const& timer);

	void _ScalingRuntime_IsRunningChanged(bool isRunning, ScalingError error);

	void _StartScale(HWND hWnd, const Profile& profile, bool windowedMode);

	void _ScaleForegroundWindow(bool windowedMode);

	ScalingError _CheckSrcWnd(HWND hWnd) noexcept;

	std::unique_ptr<ScalingRuntime> _scalingRuntime;

	winrt::DispatcherTimer _countDownTimer;
	// DispatcherTimer 在不显示主窗口时可能停滞，因此使用 ThreadPoolTimer
	winrt::Threading::ThreadPoolTimer _checkForegroundTimer{ nullptr };

	Event<winrt::Magpie::ShortcutAction>::EventRevoker _shortcutActivatedRevoker;

	std::chrono::steady_clock::time_point _timerStartTimePoint;

	uint32_t _curCountdownSeconds = 0;

	HWND _hwndCurSrc = NULL;
	// 1. 避免重复检查同一个窗口
	// 2. 用户使用热键退出全屏后暂时阻止该窗口自动放大
	// 可能在线程池中访问，因此增加原子性
	std::atomic<HWND> _hwndChecked = NULL;
	
	bool _isAutoScaling = false;
};

}

