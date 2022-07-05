#include "pch.h"
#include "ScalingRuleService.h"
#include "Win32Utils.h"


namespace winrt::Magpie::App {

ScalingRule& ScalingRuleService::GetRuleForWindow(HWND hWnd) {
	std::wstring className = Win32Utils::GetWndClassName(hWnd);
	std::wstring path;

	// 遍历规则
	for (const auto& pair : _rulesMap) {
		// 首先只匹配窗口类名
		if (pair.second->ClassNameRule() != className) {
			continue;
		}

		if (path.empty()) {
			path = Win32Utils::GetPathOfWnd(hWnd);
			if (path.empty()) {
				// 失败后不再重试
				path.resize(1, 0);
			}
		}

		if (path != pair.second->PathRule()) {
			continue;
		}

		return *pair.second;
	}

	return GetDefaultScalingRule();
}

}
