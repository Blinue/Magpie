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
#include "Win32Helper.h"
#include "Logger.h"
#include "ShortcutService.h"
#include "CommonSharedConstants.h"
#include "ScalingService.h"
#include <CoreWindow.h>
#include "EffectsService.h"
#include "UpdateService.h"
#include "LocalizationService.h"
#include "ToastService.h"
#include "NotifyIconService.h"
#include "SettingsCard.h"
#include "SettingsExpander.h"
#include "SettingsGroup.h"
#include "ControlSizeTrigger.h"
#include "IsEqualStateTrigger.h"
#include "IsNullStateTrigger.h"
#include "TextBlockHelper.h"
#include "MainWindow.h"
#include "AdaptersService.h"

using namespace ::Magpie;
using namespace winrt;
using namespace Windows::Globalization::NumberFormatting;
using namespace Windows::UI::ViewManagement;

namespace winrt::Magpie::implementation {

static UINT WM_MAGPIE_SHOWME;
static UINT WM_MAGPIE_QUIT;

static void InitMessages() noexcept {
	WM_MAGPIE_SHOWME = RegisterWindowMessage(CommonSharedConstants::WM_MAGPIE_SHOWME);
	WM_MAGPIE_QUIT = RegisterWindowMessage(CommonSharedConstants::WM_MAGPIE_QUIT);

	// 允许被权限更低的实例唤醒
	if (!ChangeWindowMessageFilter(WM_MAGPIE_SHOWME, MSGFLT_ADD)) {
		Logger::Get().Win32Error("ChangeWindowMessageFilter 失败");
	}
}

// 我们需要尽可能高的时钟分辨率来提高渲染帧率。
// 通常 Magpie 被 OS 认为是后台进程，下面的调用避免 OS 自动降低时钟分辨率。
// 见 https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-setprocessinformation
static void IncreaseTimerResolution() noexcept {
	PROCESS_POWER_THROTTLING_STATE powerThrottling{
		.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION,
		.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED |
					   PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION,
		.StateMask = 0
	};
	SetProcessInformation(
		GetCurrentProcess(),
		ProcessPowerThrottling,
		&powerThrottling,
		sizeof(powerThrottling)
	);
}

// 提前加载 twinapi.appcore.dll 和 threadpoolwinrt.dll 以避免退出时崩溃。应在 Windows.UI.Xaml.dll
// 被加载前调用，注意避免初始化全局变量时意外加载这个 dll，尤其是为了注册 DependencyProperty。
// 来自 https://github.com/CommunityToolkit/Microsoft.Toolkit.Win32/blob/6fb2c3e00803ea563af20f6bc9363091b685d81f/Microsoft.Toolkit.Win32.UI.XamlApplication/XamlApplication.cpp#L140
// 参见 https://github.com/microsoft/microsoft-ui-xaml/issues/7260#issuecomment-1231314776
static void FixThreadPoolCrash() noexcept {
	assert(!GetModuleHandle(L"Windows.UI.Xaml.dll"));
	LoadLibraryEx(L"twinapi.appcore.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
	LoadLibraryEx(L"threadpoolwinrt.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
}

App& App::Get() {
	static com_ptr<App> instance = [] {
		FixThreadPoolCrash();
		return make_self<App>();
	}();

	return *instance;
}

App::App() {
	UnhandledException([this](IInspectable const&, UnhandledExceptionEventArgs const& e) {
		Logger::Get().ComCritical("未处理的异常", e.Exception().value);

		if (IsDebuggerPresent()) {
			hstring errorMessage = e.Message();
			__debugbreak();
		}
	});
}

bool App::Initialize(const wchar_t* arguments) {
	// 提高时钟分辨率
	IncreaseTimerResolution();

	InitMessages();

	if (!_CheckSingleInstance()) {
		Logger::Get().Info("已经有一个实例正在运行");
		Logger::Get().Flush();
		return false;
	}

	_mainWindow = std::make_unique<class MainWindow>();

	EffectsService::Get().Initialize();

	// 初始化 XAML 框架。退出时也不要关闭，如果正在播放动画会崩溃。文档中的清空消息队列的做法无用。
	_windowsXamlManager = Hosting::WindowsXamlManager::InitializeForCurrentThread();

	if (CoreWindow coreWindow = CoreWindow::GetForCurrentThread()) {
		// Win10 中隐藏 DesktopWindowXamlSource 窗口
		if (!Win32Helper::GetOSVersion().IsWin11()) {
			HWND hwndDWXS;
			coreWindow.as<ICoreWindowInterop>()->get_WindowHandle(&hwndDWXS);
			ShowWindow(hwndDWXS, SW_HIDE);
		}

		_dispatcher = coreWindow.Dispatcher();
	}

	LocalizationService::Get().EarlyInitialize();

	AppSettings& settings = AppSettings::Get();
	if (!settings.Initialize()) {
		Logger::Get().Error("初始化 AppSettings 失败");
		return false;
	}

	if (settings.IsAlwaysRunAsAdmin() && !Win32Helper::IsProcessElevated()) {
		Restart(true, arguments);
		_Uninitialize();
		return false;
	}

	// 延迟注册 DependencyProperty，见 FixThreadPoolCrash
	SettingsCard::RegisterDependencyProperties();
	SettingsExpander::RegisterDependencyProperties();
	SettingsGroup::RegisterDependencyProperties();
	ControlSizeTrigger::RegisterDependencyProperties();
	IsEqualStateTrigger::RegisterDependencyProperties();
	IsNullStateTrigger::RegisterDependencyProperties();
	TextBlockHelper::RegisterDependencyProperties();

	_themeChangedRevoker = AppSettings::Get().ThemeChanged(
		auto_revoke, std::bind_front(&App::_AppSettings_ThemeChanged, this));
	_AppSettings_ThemeChanged(AppSettings::Get().Theme());

	LocalizationService::Get().Initialize();
	ToastService::Get().Initialize();
	if (!AdaptersService::Get().Initialize()) {
		_Uninitialize();
		return false;
	}
	ShortcutService::Get().Initialize();
	ScalingService::Get().Initialize();
	UpdateService::Get().Initialize();
	ThemeHelper::Initialize();

	NotifyIconService& notifyIconService = NotifyIconService::Get();
	notifyIconService.Initialize();
	notifyIconService.IsShow(AppSettings::Get().IsShowNotifyIcon());
	_isShowNotifyIconChangedRevoker = AppSettings::Get().IsShowNotifyIconChanged(
		auto_revoke, [](bool value) { NotifyIconService::Get().IsShow(value); });

	// 不显示托盘图标时忽略 -t 参数
	if (!notifyIconService.IsShow() || arguments != L"-t"sv) {
		if (!_mainWindow->Create()) {
			_Uninitialize();
			return false;
		}

		// 有的设备上后台调用 D3D11CreateDevice 会拖累主窗口显示速度，因此应在主窗口显示后
		// 再检查显卡的功能级别。
		_mainWindow->Content()->Loaded([](const auto&, const auto&) {
			// 低优先级回调确保在初始化完毕后执行
			App::Get().Dispatcher().RunAsync(CoreDispatcherPriority::Low, []() {
				AdaptersService::Get().StartMonitor();
			});
		});
	} else {
		AdaptersService::Get().StartMonitor();
	}

	return true;
}

int App::Run() {
	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0)) {
		if (msg.message == WM_MAGPIE_SHOWME) [[unlikely]] {
			ShowMainWindow();
		} else if (msg.message == WM_MAGPIE_QUIT) [[unlikely]] {
			Quit();
		} else {
			_mainWindow->HandleMessage(msg);
		}
	}

	_Uninitialize();

	Logger::Get().Info("程序退出");
	Logger::Get().Flush();

	return (int)msg.wParam;
}

void App::ShowMainWindow() noexcept {
	if (*_mainWindow) {
		_mainWindow->Show();
	} else {
		_mainWindow->Create();
	}
}

void App::Quit() {
	_mainWindow->Destroy();
	PostQuitMessage(0);
}

void App::Restart(bool asElevated, const wchar_t* arguments) noexcept {
	Quit();

	// 提前释放锁
	_ReleaseMutexes();

	// 调用 ShellExecuteEx 后立即退出，因此应该指定 SEE_MASK_NOASYNC
	SHELLEXECUTEINFO execInfo = {
		.cbSize = sizeof(execInfo),
		.fMask = SEE_MASK_NOASYNC,
		.lpVerb = asElevated ? L"runas" : L"open",
		.lpFile = Win32Helper::GetExePath().c_str(),
		.lpParameters = arguments,
		.nShow = SW_SHOWNORMAL
	};

	if (!ShellExecuteEx(&execInfo)) {
		Logger::Get().Win32Error("ShellExecuteEx 失败");
		Logger::Get().Flush();
	}
}

const com_ptr<RootPage>& App::RootPage() const noexcept {
	assert(_mainWindow && *_mainWindow);
	return _mainWindow->Content();
}

INumberFormatter2 App::DoubleFormatter() {
	static DecimalFormatter numberFormatter = []() {
		DecimalFormatter result;
		IncrementNumberRounder rounder;
		// 保留五位小数
		rounder.Increment(0.00001);
		result.NumberRounder(rounder);
		result.FractionDigits(0);
		return result;
	}();

	return numberFormatter;
}

void App::_Uninitialize() {
	NotifyIconService::Get().Uninitialize();
	UpdateService::Get().Uninitialize();
	ScalingService::Get().Uninitialize();
	// 提前取消热键注册，这样关闭 Magpie 后立即重新打开不会注册热键失败
	ShortcutService::Get().Uninitialize();
	AdaptersService::Get().Uninitialize();
	ToastService::Get().Uninitialize();
	EffectsService::Get().Uninitialize();

	_colorValuesChangedRevoker.revoke();
	_isShowNotifyIconChangedRevoker.Revoke();
	_themeChangedRevoker.Revoke();

	// 确保退出时所有事件回调都已撤销，既保持整洁又能防止析构全局变量时崩溃
	assert(_DEBUG_DELEGATE_COUNT == 0);

	_ReleaseMutexes();
}

bool App::_CheckSingleInstance() noexcept {
	static constexpr const wchar_t* ELEVATED_MUTEX_NAME = L"{E494C456-F587-4DAF-B68F-366278D31C45}";

	if (Win32Helper::IsProcessElevated()) {
		bool alreadyExists = false;
		if (!_hElevatedMutex.try_create(
			ELEVATED_MUTEX_NAME,
			CREATE_MUTEX_INITIAL_OWNER,
			MUTEX_ALL_ACCESS,
			nullptr,
			&alreadyExists) || alreadyExists) {
			// 通知已有实例显示主窗口
			PostMessage(HWND_BROADCAST, WM_MAGPIE_SHOWME, 0, 0);
			return false;
		}
	}

	bool alreadyExists = false;
	if (!_hSingleInstanceMutex.try_create(
		CommonSharedConstants::SINGLE_INSTANCE_MUTEX_NAME,
		CREATE_MUTEX_INITIAL_OWNER,
		MUTEX_ALL_ACCESS,
		nullptr,
		&alreadyExists) || alreadyExists) {
		if (_hElevatedMutex) {
			if (!_hSingleInstanceMutex) {
				return false;
			}

			// 本实例是管理员身份，而已有实例不是。通知已有实例退出
			PostMessage(HWND_BROADCAST, WM_MAGPIE_QUIT, 0, 0);

			// 等待退出完成
			if (!wil::handle_wait(_hSingleInstanceMutex.get(), 10000)) {
				Logger::Get().Error("等待超时");
				return false;
			}
		} else {
			// 通知已有实例显示主窗口
			PostMessage(HWND_BROADCAST, WM_MAGPIE_SHOWME, 0, 0);
			return false;
		}
	}

	return true;
}

void App::_AppSettings_ThemeChanged(AppTheme) {
	_UpdateColorValuesChangedRevoker();
	_UpdateTheme();
}

void App::_UpdateColorValuesChangedRevoker() {
	if (AppSettings::Get().Theme() == AppTheme::System) {
		_colorValuesChangedRevoker = _uiSettings.ColorValuesChanged(
			auto_revoke,
			[this](const auto&, const auto&) {
				_dispatcher.RunAsync(CoreDispatcherPriority::Normal, [this] {
					_UpdateTheme();
				});
			}
		);
	} else {
		_colorValuesChangedRevoker.revoke();
	}
}

// 来自 https://learn.microsoft.com/en-us/windows/apps/desktop/modernize/apply-windows-themes#know-when-dark-mode-is-enabled
static bool IsColorLight(const winrt::Windows::UI::Color& clr) noexcept {
	return 5 * clr.G + 2 * clr.R + clr.B > 8 * 128;
}

void App::_UpdateTheme() {
	AppTheme theme = AppSettings::Get().Theme();

	bool isLightTheme = false;
	if (theme == AppTheme::System) {
		// 前景色是亮色表示当前是深色主题
		isLightTheme = !IsColorLight(_uiSettings.GetColorValue(UIColorType::Foreground));
	} else {
		isLightTheme = theme == AppTheme::Light;
	}

	bool expected = !isLightTheme;
	if (_isLightTheme.compare_exchange_strong(expected, isLightTheme, std::memory_order_relaxed)) {
		ThemeChanged.Invoke(isLightTheme);
	}
}

void App::_ReleaseMutexes() noexcept {
	if (_hSingleInstanceMutex) {
		_hSingleInstanceMutex.ReleaseMutex();
		_hSingleInstanceMutex.reset();
	}
	if (_hElevatedMutex) {
		_hElevatedMutex.ReleaseMutex();
		_hElevatedMutex.reset();
	}
}

}
