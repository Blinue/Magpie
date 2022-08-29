// Copyright (c) 2021 - present, Liu Xu
//
//  This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.


#include "pch.h"
#include "App.h"
#if __has_include("App.g.cpp")
#include "App.g.cpp"
#endif
#include "Win32Utils.h"
#include "Logger.h"
#include "HotkeyService.h"
#include "AppSettings.h"
#include "CommonSharedConstants.h"
#include "MagService.h"
#include <CoreWindow.h>
#include <Magpie.Core.h>
#include "EffectsService.h"

using namespace winrt;
using namespace Windows::UI::Xaml::Media;


namespace winrt::Magpie::UI::implementation {

App::App() {
	__super::Initialize();
	
	AddRef();
	m_inner.as<::IUnknown>()->Release();

	bool isWin11 = Win32Utils::GetOSBuild() >= 22000;
	if (!isWin11) {
		// Win10 中隐藏 DesktopWindowXamlSource 窗口
		CoreWindow coreWindow = CoreWindow::GetForCurrentThread();
		if (coreWindow) {
			HWND hwndDWXS;
			coreWindow.as<ICoreWindowInterop>()->get_WindowHandle(&hwndDWXS);
			ShowWindow(hwndDWXS, SW_HIDE);
		}
	}

	// 根据操作系统版本设置样式
	ResourceDictionary resource = Resources();

	// 根据操作系统选择图标字体
	resource.Insert(
		box_value(L"SymbolThemeFontFamily"),
		FontFamily(isWin11 ? L"Segoe Fluent Icons" : L"Segoe MDL2 Assets")
	);

	if (isWin11) {
		// Win11 中更改圆角大小
		resource.Insert(
			box_value(L"ControlCornerRadius"),
			box_value(CornerRadius{ 8,8,8,8 })
		);
		resource.Insert(
			box_value(L"NavigationViewContentGridCornerRadius"),
			box_value(CornerRadius{ 8,0,0,0 })
		);
	}

	EffectsService::Get().StartInitialize();

	_displayInformation = Windows::Graphics::Display::DisplayInformation::GetForCurrentView();
}

App::~App() {
	Close();
}

void App::SaveSettings() {
	AppSettings::Get().Save();
}

StartUpOptions App::Initialize(int) {
	StartUpOptions result{};
	
	AppSettings& settings = AppSettings::Get();
	if (!settings.Initialize()) {
		result.IsError = true;
		return result;
	}

	result.IsError = false;
	result.MainWndRect = settings.WindowRect();
	result.IsWndMaximized= settings.IsWindowMaximized();
	result.IsNeedElevated = settings.IsAlwaysRunAsElevated();

	HotkeyService::Get().Initialize();
	MagService::Get().Initialize();

	return result;
}

bool App::IsShowTrayIcon() const noexcept {
	return AppSettings::Get().IsShowTrayIcon();
}

event_token App::IsShowTrayIconChanged(EventHandler<bool> const& handler) {
	return AppSettings::Get().IsShowTrayIconChanged([handler(handler)](bool value) {
		handler(nullptr, value);
	});
}

void App::IsShowTrayIconChanged(event_token const& token) {
	AppSettings::Get().IsShowTrayIconChanged(token);
}

void App::HwndMain(uint64_t value) noexcept {
	if (_hwndMain == (HWND)value) {
		return;
	}

	_hwndMain = (HWND)value;
	_hwndMainChangedEvent(*this, value);
}

void App::MainPage(Magpie::UI::MainPage const& mainPage) noexcept {
	// 显示主窗口前等待 EffectsService 完成初始化
	EffectsService::Get().WaitForInitialize();

	_mainPage = mainPage;
}

void App::OnHostWndFocusChanged(bool isFocused) {
	if (isFocused == _isHostWndFocused) {
		return;
	}

	_isHostWndFocused = isFocused;
	_hostWndFocusChangedEvent(*this, isFocused);
}

void App::RestartAsElevated() const noexcept {
	PostMessage(_hwndMain, CommonSharedConstants::WM_RESTART_AS_ELEVATED, 0, 0);
}

}
