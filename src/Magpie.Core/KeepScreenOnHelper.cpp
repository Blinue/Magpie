#include "pch.h"
#include "KeepScreenOnHelper.h"
#include "Logger.h"

namespace Magpie {

void KeepScreenOnHelper::DisableKeepScreenOn() noexcept {
	SetThreadExecutionState(ES_CONTINUOUS);
}

KeepScreenOnHelper::unique_cancel KeepScreenOnHelper::EnableKeepScreenOn() noexcept {
	unique_cancel result;

	if (SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED)) {
		Logger::Get().Info("已启用屏幕常亮");
		result.activate();
	} else {
		Logger::Get().Win32Error("SetThreadExecutionState 失败");
	}

	return result;
}

}
