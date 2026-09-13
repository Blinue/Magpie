#include "pch.h"
#include "KeepScreenOnHelper.h"
#include "Logger.h"

namespace Magpie {

void KeepScreenOnHelper::_DisableKeepScreenOn() noexcept {
	SetThreadExecutionState(ES_CONTINUOUS);
}

KeepScreenOnHelper::Guard KeepScreenOnHelper::EnableKeepScreenOn() noexcept {
	Guard result;

	if (SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED)) {
		Logger::Get().Info("已启用屏幕常亮");
		result.activate();
	} else {
		Logger::Get().Win32Error("SetThreadExecutionState 失败");
	}

	return result;
}

}
