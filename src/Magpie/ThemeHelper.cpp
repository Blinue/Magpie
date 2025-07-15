#include "pch.h"
#include "ThemeHelper.h"
#include "Win32Helper.h"
#include <dwmapi.h>

namespace Magpie {

// 这些未记录的 API 来自 https://github.com/ysc3839/win32-darkmode

enum class PreferredAppMode {
	Default,
	AllowDark,
	ForceDark,
	ForceLight,
	Max
};

using fnSetPreferredAppMode = PreferredAppMode(WINAPI*)(PreferredAppMode appMode);
using fnAllowDarkModeForWindow = bool(WINAPI*)(HWND hWnd, bool allow);
using fnRefreshImmersiveColorPolicyState = void(WINAPI*)();
using fnFlushMenuThemes = void(WINAPI*)();

static fnSetPreferredAppMode SetPreferredAppMode = nullptr;
static fnAllowDarkModeForWindow AllowDarkModeForWindow = nullptr;
static fnRefreshImmersiveColorPolicyState RefreshImmersiveColorPolicyState = nullptr;
static fnFlushMenuThemes FlushMenuThemes = nullptr;

void ThemeHelper::Initialize() noexcept {
	assert(!SetPreferredAppMode);

	const HMODULE hUxtheme = GetModuleHandle(L"uxtheme.dll");
	if (!hUxtheme) {
		return;
	}

	// 先转成 void* 以避免警告
	AllowDarkModeForWindow = reinterpret_cast<fnAllowDarkModeForWindow>(
		reinterpret_cast<void*>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(133))));
	if (!AllowDarkModeForWindow) {
		Logger::Get().Win32Error("获取 uxtheme.dll!AllowDarkModeForWindow 失败");
		return;
	}
	RefreshImmersiveColorPolicyState = reinterpret_cast<fnRefreshImmersiveColorPolicyState>(
		reinterpret_cast<void*>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(104))));
	if (!RefreshImmersiveColorPolicyState) {
		Logger::Get().Win32Error("获取 uxtheme.dll!RefreshImmersiveColorPolicyState 失败");
		return;
	}
	FlushMenuThemes = reinterpret_cast<fnFlushMenuThemes>(
		reinterpret_cast<void*>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(136))));
	if (!FlushMenuThemes) {
		Logger::Get().Win32Error("获取 uxtheme.dll!FlushMenuThemes 失败");
		return;
	}
	// 最后初始化 SetPreferredAppMode
	SetPreferredAppMode = reinterpret_cast<fnSetPreferredAppMode>(
		reinterpret_cast<void*>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135))));
	if (!SetPreferredAppMode) {
		Logger::Get().Win32Error("获取 uxtheme.dll!SetPreferredAppMode 失败");
		return;
	}

	SetPreferredAppMode(PreferredAppMode::AllowDark);
	RefreshImmersiveColorPolicyState();
}

void ThemeHelper::SetWindowTheme(HWND hWnd, bool darkBorder, bool darkMenu) noexcept {
	if (SetPreferredAppMode) {
		SetPreferredAppMode(darkMenu ? PreferredAppMode::ForceDark : PreferredAppMode::ForceLight);
		AllowDarkModeForWindow(hWnd, darkMenu);
	}

	// 使标题栏适应黑暗模式
	// build 18985 之前 DWMWA_USE_IMMERSIVE_DARK_MODE 的值不同
	// https://github.com/MicrosoftDocs/sdk-api/pull/966/files
	static constexpr DWORD DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 = 19;
	BOOL value = darkBorder;
	DwmSetWindowAttribute(
		hWnd,
		Win32Helper::GetOSVersion().Is20H1OrNewer() ? DWMWA_USE_IMMERSIVE_DARK_MODE : DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1,
		&value,
		sizeof(value)
	);

	if (SetPreferredAppMode) {
		RefreshImmersiveColorPolicyState();
		FlushMenuThemes();
	}
}

}
