#include "pch.h"
#include "ScalingProfileService.h"
#include "Win32Utils.h"
#include "AppSettings.h"


namespace winrt::Magpie::App {

ScalingProfile& ScalingProfileService::GetProfileForWindow(HWND hWnd) {
	std::wstring className = Win32Utils::GetWndClassName(hWnd);
	std::wstring path;

	// 遍历规则
	for (ScalingProfile& rule : AppSettings::Get().ScalingProfiles()) {
		// 首先只匹配窗口类名
		if (rule.ClassNameRule() != className) {
			continue;
		}

		if (path.empty()) {
			path = Win32Utils::GetPathOfWnd(hWnd);
			if (path.empty()) {
				// 失败后不再重试
				path.resize(1, 0);
			}
		}

		if (path != rule.PathRule()) {
			continue;
		}

		return rule;
	}

	return GetDefaultScalingProfile();
}

ScalingProfile& ScalingProfileService::GetDefaultScalingProfile() {
	return AppSettings::Get().DefaultScalingProfile();
}

}
