#include "pch.h"
#include "ScalingProfileService.h"
#include "Win32Utils.h"
#include "AppSettings.h"
#include "AppXReader.h"


namespace winrt::Magpie::App {
bool ScalingProfileService::TestNewProfile(bool isPackaged, std::wstring_view pathOrAumid, std::wstring_view className) {
	if (pathOrAumid.empty() || className.empty()) {
		return false;
	}

	std::vector<ScalingProfile>& profiles = AppSettings::Get().ScalingProfiles();
	if (isPackaged) {
		for (ScalingProfile& rule : profiles) {
			if (rule.IsPackaged() && rule.PathRule() == pathOrAumid && rule.ClassNameRule() == className) {
				return false;
			}
		}
	} else {
		for (ScalingProfile& rule : profiles) {
			if (!rule.IsPackaged() && rule.PathRule() == pathOrAumid && rule.ClassNameRule() == className) {
				return false;
			}
		}
	}

	return true;
}
bool ScalingProfileService::AddProfile(bool isPackaged, std::wstring_view pathOrAumid, std::wstring_view className, std::wstring_view name) {
	if (!TestNewProfile(isPackaged, pathOrAumid, className) || name.empty()) {
		return false;
	}

	ScalingProfile& profile = AppSettings::Get().ScalingProfiles().emplace_back();
	profile.IsPackaged(isPackaged);
	profile.PathRule(pathOrAumid);
	profile.ClassNameRule(className);
	profile.Name(name);

	_profileAddedEvent(std::ref(profile));

	return true;
}

ScalingProfile& ScalingProfileService::GetProfileForWindow(HWND hWnd) {
	std::wstring className = Win32Utils::GetWndClassName(hWnd);

	AppXReader appXReader;
	if (appXReader.Initialize(hWnd)) {
		// 打包的应用程序匹配 AUMID 和 类名
		const std::wstring& aumid = appXReader.AUMID();
		for (ScalingProfile& rule : AppSettings::Get().ScalingProfiles()) {
			if (rule.IsPackaged() && rule.PathRule() == aumid && rule.ClassNameRule() == className) {
				return rule;
			}
		}
	} else {
		// 桌面程序匹配类名和可执行文件名
		std::wstring path = Win32Utils::GetPathOfWnd(hWnd);

		for (ScalingProfile& rule : AppSettings::Get().ScalingProfiles()) {
			if (!rule.IsPackaged() && rule.PathRule() == path && rule.ClassNameRule() == className) {
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
