#include "pch.h"
#include "ScalingProfileService.h"
#include "Win32Utils.h"
#include "AppSettings.h"
#include "AppXReader.h"
#include <regex>


namespace winrt::Magpie::App {

static std::wstring_view GetRealClassName(std::wstring_view className) {
	// WPF 窗口类每次启动都会改变，格式为：
	// HwndWrapper[{名称};;{GUID}]
	// GUID 格式为 xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
	static std::wregex wpfClassNameRegex(LR"(^HwndWrapper\[(.*);;[0-9,a-f]{8}-[0-9,a-f]{4}-[0-9,a-f]{4}-[0-9,a-f]{4}-[0-9,a-f]{12}\]$)", std::wregex::optimize);

	std::match_results<std::wstring_view::iterator> matchResults;
	if (std::regex_match(className.begin(), className.end(), matchResults, wpfClassNameRegex) && matchResults.size() == 2) {
		return { matchResults[1].first, matchResults[1].second };
	} else {
		return className;
	}
}

static bool RealTestNewProfile(
	bool isPackaged,
	std::wstring_view pathOrAumid,
	std::wstring_view realClassName
) {
	const std::vector<ScalingProfile>& profiles = AppSettings::Get().ScalingProfiles();

	if (isPackaged) {
		for (const ScalingProfile& rule : profiles) {
			if (rule.IsPackaged() && rule.PathRule() == pathOrAumid && rule.ClassNameRule() == realClassName) {
				return false;
			}
		}
	} else {
		for (const ScalingProfile& rule : profiles) {
			if (!rule.IsPackaged() && rule.PathRule() == pathOrAumid && rule.ClassNameRule() == realClassName) {
				return false;
			}
		}
	}

	return true;
}

bool ScalingProfileService::TestNewProfile(bool isPackaged, std::wstring_view pathOrAumid, std::wstring_view className) {
	if (pathOrAumid.empty() || className.empty()) {
		return false;
	}

	return RealTestNewProfile(isPackaged, pathOrAumid, GetRealClassName(className));
}

bool ScalingProfileService::AddProfile(bool isPackaged, std::wstring_view pathOrAumid, std::wstring_view className, std::wstring_view name) {
	if (pathOrAumid.empty() || className.empty() || name.empty()) {
		return false;
	}

	std::wstring_view realClassName = GetRealClassName(className);

	if (!RealTestNewProfile(isPackaged, pathOrAumid, realClassName)) {
		return false;
	}

	ScalingProfile& profile = AppSettings::Get().ScalingProfiles().emplace_back();
	profile.IsPackaged(isPackaged);
	profile.PathRule(pathOrAumid);
	profile.ClassNameRule(realClassName);
	profile.Name(name);

	_profileAddedEvent(std::ref(profile));

	return true;
}

ScalingProfile& ScalingProfileService::GetProfileForWindow(HWND hWnd) {
	std::wstring className = Win32Utils::GetWndClassName(hWnd);
	std::wstring_view realClassName = GetRealClassName(className);

	AppXReader appXReader;
	if (appXReader.Initialize(hWnd)) {
		// 打包的应用程序匹配 AUMID 和 类名
		const std::wstring& aumid = appXReader.AUMID();
		for (ScalingProfile& rule : AppSettings::Get().ScalingProfiles()) {
			if (rule.IsPackaged() && rule.PathRule() == aumid && rule.ClassNameRule() == realClassName) {
				return rule;
			}
		}
	} else {
		// 桌面程序匹配类名和可执行文件名
		std::wstring path = Win32Utils::GetPathOfWnd(hWnd);

		for (ScalingProfile& rule : AppSettings::Get().ScalingProfiles()) {
			if (!rule.IsPackaged() && rule.PathRule() == path && rule.ClassNameRule() == realClassName) {
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
