#include "pch.h"
#include "ProfileService.h"
#include "Win32Utils.h"
#include "AppSettings.h"
#include "AppXReader.h"
#include <regex>
#include "StrUtils.h"

namespace winrt::Magpie::App {

// WPF 窗口类每次启动都会改变，格式为:
// HwndWrapper[{名称};;{GUID}]
// GUID 格式为 xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
static bool MatchWPFClassName(std::wstring_view& className) noexcept {
	static constexpr const wchar_t* WPF_PREFIX = L"HwndWrapper[";
	static constexpr const wchar_t* WPF_SUFFIX = L"]";
	if (!className.starts_with(WPF_PREFIX) || !className.ends_with(WPF_SUFFIX)) {
		return false;
	}

	static const std::wregex regex(
		LR"(^(.*);;[0-9,a-f]{8}-[0-9,a-f]{4}-[0-9,a-f]{4}-[0-9,a-f]{4}-[0-9,a-f]{12}$)",
		std::wregex::optimize
	);

	std::match_results<std::wstring_view::iterator> matchResults;
	if (!std::regex_match(
		className.begin() + StrUtils::StrLen(WPF_PREFIX),
		className.end() - StrUtils::StrLen(WPF_SUFFIX),
		matchResults,
		regex
	)) {
		return false;
	}

	if (matchResults.size() != 2) {
		return false;
	}

	className = std::wstring_view(matchResults[1].first, matchResults[1].second);
	return true;
}

// GH#508
// RPG Maker MZ 制作的游戏每次重新加载（快捷键 F5）窗口类名都会改变，格式为:
// Chrome_WidgetWin_{递增的数字}
// 这个类名似乎在基于 Chromium 的程序中很常见，大多数时候是 Chrome_WidgetWin_1
static bool MatchRPGMakerMZClassName(std::wstring_view& className) noexcept {
	static constexpr const wchar_t* RPG_MAKER_MZ_PREFIX = L"Chrome_WidgetWin_";
	if (!className.starts_with(RPG_MAKER_MZ_PREFIX)) {
		return false;
	}

	// 检查数字后缀
	for (wchar_t c : wil::make_range(className.begin() + StrUtils::StrLen(RPG_MAKER_MZ_PREFIX), className.end())) {
		if (!StrUtils::isdigit(c)) {
			return false;
		}
	}

	className = L"Chrome_WidgetWin_1";
	return true;
}

// GH#904
// TeknoParrot 模拟 Linux 游戏时创建的窗口类名格式为：
// XWindow_{一串数字}
static bool MatchTeknoParrotClassName(std::wstring_view& className) noexcept {
	static constexpr const wchar_t* TEKNO_PARROT_PREFIX = L"XWindow_";
	if (!className.starts_with(TEKNO_PARROT_PREFIX)) {
		return false;
	}

	// 检查数字后缀
	for (wchar_t c : wil::make_range(className.begin() + StrUtils::StrLen(TEKNO_PARROT_PREFIX), className.end())) {
		if (!StrUtils::isdigit(c)) {
			return false;
		}
	}

	className = L"XWindow_0";
	return true;
}

static std::wstring_view ParseClassName(std::wstring_view className) noexcept {
	for (auto func : {
		MatchWPFClassName,
		MatchRPGMakerMZClassName,
		MatchTeknoParrotClassName
	}) {
		if (func(className)) {
			return className;
		}
	}

	return className;
}

static bool TestNewProfileImpl(
	bool isPackaged,
	std::wstring_view pathOrAumid,
	std::wstring_view parsedClassName
) noexcept {
	const std::vector<Profile>& profiles = AppSettings::Get().Profiles();

	if (isPackaged) {
		for (const Profile& rule : profiles) {
			if (rule.isPackaged && rule.pathRule == pathOrAumid && rule.classNameRule == parsedClassName) {
				return false;
			}
		}
	} else {
		for (const Profile& rule : profiles) {
			if (!rule.isPackaged && rule.pathRule == pathOrAumid && rule.classNameRule == parsedClassName) {
				return false;
			}
		}
	}

	return true;
}

bool ProfileService::TestNewProfile(bool isPackaged, std::wstring_view pathOrAumid, std::wstring_view className) noexcept {
	if (pathOrAumid.empty() || className.empty()) {
		return false;
	}

	return TestNewProfileImpl(isPackaged, pathOrAumid, ParseClassName(className));
}

bool ProfileService::AddProfile(
	bool isPackaged,
	std::wstring_view pathOrAumid,
	std::wstring_view className,
	std::wstring_view name,
	int copyFrom
) {
	assert(!pathOrAumid.empty() && !className.empty() && !name.empty());

	const std::wstring_view parsedClassName = ParseClassName(className);

	if (!TestNewProfileImpl(isPackaged, pathOrAumid, parsedClassName)) {
		return false;
	}

	std::vector<Profile>& profiles = AppSettings::Get().Profiles();
	Profile& profile = profiles.emplace_back();

	profile.Copy(copyFrom < 0 ? DefaultProfile() : profiles[copyFrom]);

	profile.name = name;
	profile.isPackaged = isPackaged;
	profile.pathRule = pathOrAumid;
	profile.classNameRule = parsedClassName;

	ProfileAdded.Invoke(std::ref(profile));

	AppSettings::Get().SaveAsync();
	return true;
}

void ProfileService::RenameProfile(uint32_t profileIdx, std::wstring_view newName) {
	assert(!newName.empty());
	AppSettings::Get().Profiles()[profileIdx].name = newName;
	ProfileRenamed.Invoke(profileIdx);
	AppSettings::Get().SaveAsync();
}

void ProfileService::RemoveProfile(uint32_t profileIdx) {
	std::vector<Profile>& profiles = AppSettings::Get().Profiles();
	profiles.erase(profiles.begin() + profileIdx);
	ProfileRemoved.Invoke(profileIdx);
	AppSettings::Get().SaveAsync();
}

bool ProfileService::MoveProfile(uint32_t profileIdx, bool isMoveUp) {
	std::vector<Profile>& profiles = AppSettings::Get().Profiles();
	if (isMoveUp ? profileIdx == 0 : profileIdx + 1 >= (uint32_t)profiles.size()) {
		return false;
	}

	std::swap(profiles[profileIdx], profiles[isMoveUp ? (size_t)profileIdx - 1 : (size_t)profileIdx + 1]);
	ProfileMoved.Invoke(profileIdx, isMoveUp);

	AppSettings::Get().SaveAsync();
	return true;
}

static bool AnyAutoScaleProfile(const std::vector<Profile>& profiles) noexcept {
	for (const Profile& profile : profiles) {
		if (profile.isAutoScale) {
			return true;
		}
	}
	return false;
}

const Profile* ProfileService::GetProfileForWindow(HWND hWnd, bool forAutoScale) noexcept {
	const std::vector<Profile>& profiles = AppSettings::Get().Profiles();

	// 作为优化，先检查有没有配置文件启用了自动缩放
	if (forAutoScale && !AnyAutoScaleProfile(profiles)) {
		return nullptr;
	}
	
	// 先检查窗口类名，这比获取可执行文件名快得多
	std::wstring className = Win32Utils::GetWndClassName(hWnd);
	std::wstring_view parsedClassName = ParseClassName(className);

	std::wstring path;
	std::optional<bool> isPackaged;

	for (const Profile& profile : profiles) {
		if (forAutoScale && !profile.isAutoScale) {
			continue;
		}

		if (profile.classNameRule != parsedClassName) {
			continue;
		}

		if (!isPackaged.has_value()) {
			AppXReader appxReader;
			isPackaged = appxReader.Initialize(hWnd);
			if (*isPackaged) {
				// 打包应用匹配 AUMID
				path = appxReader.AUMID();
			}
		}

		if (profile.isPackaged != *isPackaged) {
			continue;
		}

		if (!*isPackaged && path.empty()) {
			// 桌面应用匹配路径
			path = Win32Utils::GetPathOfWnd(hWnd);
			if (path.empty()) {
				// 获取路径失败
				break;
			}
		}
		
		if (profile.pathRule == path) {
			return &profile;
		}
	}

	return forAutoScale ? nullptr : &DefaultProfile();
}

Profile& ProfileService::DefaultProfile() noexcept {
	return AppSettings::Get().DefaultProfile();
}

Profile& ProfileService::GetProfile(uint32_t idx) noexcept {
	return AppSettings::Get().Profiles()[idx];
}

uint32_t ProfileService::GetProfileCount() noexcept {
	return (uint32_t)AppSettings::Get().Profiles().size();
}

}
