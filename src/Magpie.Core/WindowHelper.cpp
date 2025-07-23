#include "pch.h"
#include "WindowHelper.h"
#include "StrHelper.h"
#include "Win32Helper.h"
#include <parallel_hashmap/phmap.h>

namespace Magpie {

static const wchar_t* CoreWindowClassName = L"Windows.UI.Core.CoreWindow";

bool WindowHelper::IsStartMenu(HWND hWnd) noexcept {
	// 作为优化，首先检查窗口类
	std::wstring className = Win32Helper::GetWindowClassName(hWnd);

	if (className != CoreWindowClassName) {
		return false;
	}

	// 检查可执行文件名称
	std::wstring exeName = Win32Helper::GetWindowExeName(hWnd);
	StrHelper::ToLowerCase(exeName);
	// win10: searchapp.exe 和 startmenuexperiencehost.exe
	// win11: searchhost.exe 和 startmenuexperiencehost.exe
	return exeName == L"searchapp.exe" || exeName == L"searchhost.exe" ||
		exeName == L"startmenuexperiencehost.exe";
}

bool WindowHelper::IsForbiddenSystemWindow(HWND hwndSrc) noexcept {
	// (可执行文件名, 类名)
	static const phmap::flat_hash_set<std::pair<std::wstring_view, std::wstring_view>> systemWindows{
		{ L"explorer.exe", L"Shell_TrayWnd" },							// 任务栏
		{ L"explorer.exe", L"NotifyIconOverflowWindow" },				// 任务栏通知区域溢出菜单
		{ L"explorer.exe", L"TopLevelWindowForOverflowXamlIsland" },	// 任务栏通知区域溢出菜单 (Win11)
		{ L"explorer.exe", L"WorkerW" },								// 桌面窗口
		{ L"explorer.exe", L"Progman" },								// 桌面窗口
		{ L"explorer.exe", L"ForegroundStaging" },						// 任务视图
		{ L"explorer.exe", L"MultitaskingViewFrame" },					// 任务视图 (Win10)
		{ L"explorer.exe", L"XamlExplorerHostIslandWindow" },			// 任务视图 (Win11)
		{ L"startmenuexperiencehost.exe", CoreWindowClassName },		// 开始菜单
		{ L"searchapp.exe", CoreWindowClassName },						// 开始菜单 (Win10)
		{ L"searchhost.exe", CoreWindowClassName },						// 开始菜单 (Win11)
		{ L"shellexperiencehost.exe", CoreWindowClassName }				// 任务中心
	};

	std::wstring exeName = Win32Helper::GetWindowExeName(hwndSrc);
	StrHelper::ToLowerCase(exeName);
	
	return systemWindows.contains(std::make_pair(
		exeName, Win32Helper::GetWindowClassName(hwndSrc)));
}

}
