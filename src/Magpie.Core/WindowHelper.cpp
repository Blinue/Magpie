#include "pch.h"
#include "WindowHelper.h"
#include "Win32Utils.h"
#include "StrUtils.h"

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

bool WindowHelper::IsValidSrcWindow(HWND hwndSrc) noexcept {
	std::wstring className = Win32Utils::GetWndClassName(hwndSrc);

	// 禁止缩放特殊的系统窗口
	if (className == L"WorkerW" || className == L"ForegroundStaging" ||
		className == L"MultitaskingViewFrame" || className == L"XamlExplorerHostIslandWindow" ||
		className == L"Shell_TrayWnd" || className == L"NotifyIconOverflowWindow" ||
		className == L"Progman"
	) {
		return false;
	}

	if (className != L"Windows.UI.Core.CoreWindow") {
		return true;
	}

	// 检查可执行文件名称
	std::wstring exeName = GetExeName(hwndSrc);
	// 禁止缩放开始菜单和任务中心
	return exeName != L"searchapp.exe" && exeName != L"searchhost.exe" &&
		exeName != L"startmenuexperiencehost.exe" && exeName != L"shellexperiencehost.exe";
}

}
