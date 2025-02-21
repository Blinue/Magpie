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

[[maybe_unused]]
static bool IsInitialized() noexcept {
	return SetPreferredAppMode;
}

void ThemeHelper::Initialize() noexcept {
	assert(!IsInitialized());

	HMODULE hUxtheme = GetModuleHandle(L"uxtheme.dll");
	assert(hUxtheme);

	SetPreferredAppMode = (fnSetPreferredAppMode)GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135));
	AllowDarkModeForWindow = (fnAllowDarkModeForWindow)GetProcAddress(hUxtheme, MAKEINTRESOURCEA(133));
	RefreshImmersiveColorPolicyState = (fnRefreshImmersiveColorPolicyState)GetProcAddress(hUxtheme, MAKEINTRESOURCEA(104));
	FlushMenuThemes = (fnFlushMenuThemes)GetProcAddress(hUxtheme, MAKEINTRESOURCEA(136));

	SetPreferredAppMode(PreferredAppMode::AllowDark);
	RefreshImmersiveColorPolicyState();
}

void ThemeHelper::SetWindowTheme(HWND hWnd, bool darkBorder, bool darkMenu) noexcept {
	assert(IsInitialized());

	SetPreferredAppMode(darkMenu ? PreferredAppMode::ForceDark : PreferredAppMode::ForceLight);
	AllowDarkModeForWindow(hWnd, darkMenu);

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

	RefreshImmersiveColorPolicyState();
	FlushMenuThemes();
}

}
