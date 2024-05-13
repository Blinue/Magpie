#include "pch.h"
#include "ExclModeHelper.h"
#include "Logger.h"

namespace Magpie::Core {

// 模拟 D3D 独占全屏模式，以起到免打扰的效果
// SHQueryUserNotificationState 通常被用来检测是否有 D3D 游戏独占全屏，以确定是否应该向用户推送通知/弹窗
// 此函数内部使用名为 __DDrawExclMode__ 的 mutex 检测独占全屏，因此这里直接获取该 mutex 以模拟独占全屏
// 感谢 @codehz 提供的思路 GH#245
wil::unique_mutex_nothrow ExclModeHelper::EnterExclMode() noexcept {
	wil::unique_mutex_nothrow exclModeMutex;

	QUERY_USER_NOTIFICATION_STATE state;
	HRESULT hr = SHQueryUserNotificationState(&state);
	if (FAILED(hr)) {
		Logger::Get().ComError("SHQueryUserNotificationState 失败", hr);
		return exclModeMutex;
	}

	// 操作系统将 Magpie 的缩放窗口视为全屏应用程序，可能已经启用了“请勿打扰”，即 QUNS_BUSY。
	// 但我们想要的是 QUNS_RUNNING_D3D_FULL_SCREEN
	if (state == QUNS_RUNNING_D3D_FULL_SCREEN) {
		Logger::Get().Info("已处于免打扰状态");
		return exclModeMutex;
	}
	
	if (!exclModeMutex.try_open(L"__DDrawExclMode__", SYNCHRONIZE)) {
		Logger::Get().Win32Error("OpenMutex 失败");
		return exclModeMutex;
	}

	if (!wil::event_is_signaled(exclModeMutex.get())) {
		Logger::Get().Error("获取 __DDrawExclMode__ 失败");
		exclModeMutex.reset();
		return exclModeMutex;
	}

	hr = SHQueryUserNotificationState(&state);
	if (FAILED(hr)) {
		Logger::Get().ComError("SHQueryUserNotificationState 失败", hr);
		exclModeMutex.reset();
		return exclModeMutex;
	}
	if (state != QUNS_RUNNING_D3D_FULL_SCREEN) {
		Logger::Get().Error("模拟独占全屏失败");
		exclModeMutex.reset();
		return exclModeMutex;
	}

	Logger::Get().Info("模拟独占全屏成功");
	return exclModeMutex;
}

}
