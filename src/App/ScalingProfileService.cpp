#include "pch.h"
#include "ScalingProfileService.h"
#include "Win32Utils.h"
#include "AppSettings.h"
#include "AppXReader.h"


namespace winrt::Magpie::App {

ScalingProfile& ScalingProfileService::GetProfileForWindow(HWND hWnd) {
	std::wstring className = Win32Utils::GetWndClassName(hWnd);

	AppXReader appXReader;
	if (appXReader.Initialize(hWnd)) {
		// 打包的应用程序
		const std::wstring& aumid = appXReader.AUMID();
		for (ScalingProfile& rule : AppSettings::Get().ScalingProfiles()) {
			if (rule.IsPackaged() && rule.PathRule() == aumid && rule.ClassNameRule() == className) {
				return rule;
			}
		}
	} else {
		std::wstring path = Win32Utils::GetPathOfWnd(hWnd);

		for (ScalingProfile& rule : AppSettings::Get().ScalingProfiles()) {
			if (rule.ClassNameRule() == className && path == rule.PathRule()) {
				return rule;
			}
		}
	}

	return GetDefaultScalingProfile();
}

ScalingProfile& ScalingProfileService::GetDefaultScalingProfile() {
	return AppSettings::Get().DefaultScalingProfile();
}

}
