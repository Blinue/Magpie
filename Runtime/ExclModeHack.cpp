#include "pch.h"
#include "ExclModeHack.h"
#include "App.h"
#include <shellapi.h>


// 模拟 D3D 独占全屏模式，以起到免打扰的效果
// SHQueryUserNotificationState 通常被用来检测是否有 D3D 游戏独占全屏，以确定是否应该向用户推送通知/弹窗
// 此函数内部使用名为 __DDrawExclMode__ 的 mutex 检测独占全屏，因此这里直接获取该 mutex 以模拟独占全屏
// 感谢 @codehz 提供的思路 https://github.com/Blinue/Magpie/issues/245
ExclModeHack::ExclModeHack() {
	if (!App::GetInstance().IsSimulateExclusiveFullscreen()) {
		return;
	}

	QUERY_USER_NOTIFICATION_STATE state;
	HRESULT hr = SHQueryUserNotificationState(&state);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("SHQueryUserNotificationState 失败", hr));
		return;
	}
	if (state != QUNS_ACCEPTS_NOTIFICATIONS) {
		SPDLOG_LOGGER_INFO(logger, "已处于免打扰状态");
		return;
	}

	_exclModeMutex.reset(Utils::SafeHandle(
		OpenMutex(SYNCHRONIZE, FALSE, L"__DDrawExclMode__")));
	if (!_exclModeMutex) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("OpenMutex 失败"));
		return;
	}

	DWORD result = WaitForSingleObject(_exclModeMutex.get(), 0);
	if (result != WAIT_OBJECT_0) {
		SPDLOG_LOGGER_ERROR(logger, "获取 __DDrawExclMode__ 失败");
		_exclModeMutex.reset();
		return;
	}

	hr = SHQueryUserNotificationState(&state);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("SHQueryUserNotificationState 失败", hr));
		ReleaseMutex(_exclModeMutex.get());
		_exclModeMutex.reset();
		return;
	}
	if (state != QUNS_RUNNING_D3D_FULL_SCREEN) {
		SPDLOG_LOGGER_INFO(logger, "模拟独占全屏失败");
		ReleaseMutex(_exclModeMutex.get());
		_exclModeMutex.reset();
		return;
	}

	SPDLOG_LOGGER_INFO(logger, "模拟独占全屏成功");
}

ExclModeHack::~ExclModeHack() {
	if (_exclModeMutex) {
		ReleaseMutex(_exclModeMutex.get());
	}
}
