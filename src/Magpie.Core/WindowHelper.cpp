#include "pch.h"
#include "WindowHelper.h"
#include "Win32Utils.h"
#include "StrUtils.h"
#include <parallel_hashmap/phmap.h>

namespace Magpie::Core {

static std::wstring GetExeName(HWND hWnd) noexcept {
	std::wstring exeName = Win32Utils::GetPathOfWnd(hWnd);
	exeName = exeName.substr(exeName.find_last_of(L'\\') + 1);
	StrUtils::ToLowerCase(exeName);
	return exeName;
}

bool WindowHelper::IsStartMenu(HWND hWnd) noexcept {
	// 作为优化，首先检查窗口类
	std::wstring className = Win32Utils::GetWndClassName(hWnd);

	if (className != L"Windows.UI.Core.CoreWindow") {
		return false;
	}

	// 检查可执行文件名称
	std::wstring exeName = GetExeName(hWnd);
	// win10: searchapp.exe 和 startmenuexperiencehost.exe
	// win11: searchhost.exe 和 startmenuexperiencehost.exe
	return exeName == L"searchapp.exe" || exeName == L"searchhost.exe" ||
		exeName == L"startmenuexperiencehost.exe";
}

bool WindowHelper::IsForbiddenSystemWindow(HWND hwndSrc) noexcept {
	// 禁止缩放的系统窗口
	// (可执行文件名, 类名)
	static const phmap::flat_hash_set<std::pair<std::wstring_view, std::wstring_view>> systemWindows{
		{ L"explorer.exe", L"Shell_TrayWnd" },		// 任务栏
		{ L"explorer.exe", L"NotifyIconOverflowWindow" },				// 任务栏通知区域溢出菜单
		{ L"explorer.exe", L"TopLevelWindowForOverflowXamlIsland" },	// 任务栏通知区域溢出菜单 (Win11)
		{ L"explorer.exe", L"WorkerW" },	// 桌面窗口
		{ L"explorer.exe", L"Progman" },	// 桌面窗口
		{ L"explorer.exe", L"ForegroundStaging" },				// 任务视图
		{ L"explorer.exe", L"XamlExplorerHostIslandWindow" },	// 任务视图 (Win11)
		{ L"explorer.exe", L"MultitaskingViewFrame" },			// 任务视图 (Win10)
		{ L"startmenuexperiencehost.exe", L"Windows.UI.Core.CoreWindow" },	// 开始菜单
		{ L"searchapp.exe", L"Windows.UI.Core.CoreWindow" },				// 开始菜单 (Win10)
		{ L"searchhost.exe", L"Windows.UI.Core.CoreWindow" },				// 开始菜单 (Win11)
		{ L"shellexperiencehost.exe", L"Windows.UI.Core.CoreWindow" }		// 任务中心
	};
	
	return systemWindows.contains(std::make_pair(
		GetExeName(hwndSrc), Win32Utils::GetWndClassName(hwndSrc)));
}

}
