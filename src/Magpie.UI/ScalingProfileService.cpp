#include "pch.h"
#include "ScalingProfileService.h"
#include "Win32Utils.h"
#include "AppSettings.h"
#include "AppXReader.h"
#include <regex>


namespace winrt::Magpie::UI {

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
			if (rule.isPackaged && rule.pathRule == pathOrAumid && rule.classNameRule == realClassName) {
				return false;
			}
		}
	} else {
		for (const ScalingProfile& rule : profiles) {
			if (!rule.isPackaged && rule.pathRule == pathOrAumid && rule.classNameRule == realClassName) {
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

bool ScalingProfileService::AddProfile(
	bool isPackaged,
	std::wstring_view pathOrAumid,
	std::wstring_view className,
	std::wstring_view name,
	int copyFrom
) {
	if (pathOrAumid.empty() || className.empty() || name.empty()) {
		return false;
	}

	std::wstring_view realClassName = GetRealClassName(className);

	if (!RealTestNewProfile(isPackaged, pathOrAumid, realClassName)) {
		return false;
	}

	std::vector<ScalingProfile>& profiles = AppSettings::Get().ScalingProfiles();
	ScalingProfile& profile = profiles.emplace_back();

	profile.Copy(copyFrom < 0 ? DefaultScalingProfile() : profiles[copyFrom]);

	profile.name = name;
	profile.isPackaged = isPackaged;
	profile.pathRule = pathOrAumid;
	profile.classNameRule = realClassName;

	_profileAddedEvent(std::ref(profile));

	return true;
}

void ScalingProfileService::RenameProfile(uint32_t profileIdx, std::wstring_view newName) {
	assert(!newName.empty());
	AppSettings::Get().ScalingProfiles()[profileIdx].name = newName;
	_profileRenamedEvent(profileIdx);
}

void ScalingProfileService::RemoveProfile(uint32_t profileIdx) {
	std::vector<ScalingProfile>& profiles = AppSettings::Get().ScalingProfiles();
	profiles.erase(profiles.begin() + profileIdx);
	_profileRemovedEvent(profileIdx);
}

bool ScalingProfileService::ReorderProfile(uint32_t profileIdx, bool isMoveUp) {
	std::vector<ScalingProfile>& profiles = AppSettings::Get().ScalingProfiles();
	if (isMoveUp ? profileIdx == 0 : profileIdx + 1 >= (uint32_t)profiles.size()) {
		return false;
	}

	std::swap(profiles[profileIdx], profiles[isMoveUp ? (size_t)profileIdx - 1 : (size_t)profileIdx + 1]);
	_profileReorderedEvent(profileIdx, isMoveUp);
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
			if (rule.isPackaged && rule.pathRule == aumid && rule.classNameRule == realClassName) {
				return rule;
			}
		}
	} else {
		// 桌面程序匹配类名和可执行文件名
		std::wstring path = Win32Utils::GetPathOfWnd(hWnd);

		for (ScalingProfile& rule : AppSettings::Get().ScalingProfiles()) {
			if (!rule.isPackaged && rule.pathRule == path && rule.classNameRule == realClassName) {
				return rule;
			}
		}
	}

	return DefaultScalingProfile();
}

ScalingProfile& ScalingProfileService::DefaultScalingProfile() {
	return AppSettings::Get().DefaultScalingProfile();
}

ScalingProfile& ScalingProfileService::GetProfile(uint32_t idx) {
	return AppSettings::Get().ScalingProfiles()[idx];
}

uint32_t ScalingProfileService::GetProfileCount() {
	return (uint32_t)AppSettings::Get().ScalingProfiles().size();
}

}
