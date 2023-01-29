// Copyright (c) 2021 - present, Liu Xu
//
// This program is free software: you can redistribute it and/or modify
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
#include "UpdateService.h"

using namespace winrt;
using namespace Windows::UI::Xaml::Media;

namespace winrt::Magpie::App::implementation {

App::App() {
	EffectsService::Get().StartInitialize();

	// 初始化 XAML 框架
	_windowsXamlManager = Hosting::WindowsXamlManager::InitializeForCurrentThread();

	const bool isWin11 = Win32Utils::GetOSVersion().IsWin11();
	if (!isWin11) {
		// Win10 中隐藏 DesktopWindowXamlSource 窗口
		if (CoreWindow coreWindow = CoreWindow::GetForCurrentThread()) {
			HWND hwndDWXS;
			coreWindow.as<ICoreWindowInterop>()->get_WindowHandle(&hwndDWXS);
			ShowWindow(hwndDWXS, SW_HIDE);
		}
	}

	// 根据操作系统选择图标字体
	Resources().Insert(
		box_value(L"SymbolThemeFontFamily"),
		FontFamily(isWin11 ? L"Segoe Fluent Icons" : L"Segoe MDL2 Assets")
	);
}

App::~App() {
	Close();
}

void App::Close() {
	if (_isClosed) {
		return;
	}
	_isClosed = true;

	_windowsXamlManager.Close();
	_windowsXamlManager = nullptr;

	// 不显示托盘图标的情况下关闭主窗口仍会在后台驻留数秒，推测和 XAML Islands 有关
	// 这里提前取消热键注册，这样关闭 Magpie 后立即重新打开不会注册热键失败
	HotkeyService::Get().Destory();

	Exit();
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
	const RECT& windowRect = settings.WindowRect();
	result.MainWndRect = {
		(float)windowRect.left,
		(float)windowRect.top,
		(float)windowRect.right,
		(float)windowRect.bottom
	};
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

void App::MainPage(Magpie::App::MainPage const& mainPage) noexcept {
	// 显示主窗口前等待 EffectsService 完成初始化
	EffectsService::Get().WaitForInitialize();

	if (mainPage) {
		// 不存储对 MainPage 的强引用
		// XAML Islands 内部保留着对 MainPage 的强引用，MainPage 的生命周期是无法预知的
		_mainPage = weak_ref(mainPage);
	} else {
		UpdateService::Get().ClosingMainWindow();
	}
}

void App::RestartAsElevated() const noexcept {
	PostMessage(_hwndMain, CommonSharedConstants::WM_RESTART_AS_ELEVATED, 0, 0);
}

void App::Quit() const noexcept {
	PostMessage(_hwndMain, CommonSharedConstants::WM_QUIT_MAGPIE, 0, 0);
}

}
