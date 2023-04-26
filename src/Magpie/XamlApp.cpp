#include "pch.h"
#include "XamlApp.h"
#include "Logger.h"
#include "Win32Utils.h"
#include "CommonSharedConstants.h"
#include <fmt/xchar.h>
#include <Magpie.Core.h>
#include "ThemeHelper.h"
#include "TrayIconService.h"

namespace Magpie {

static const UINT WM_MAGPIE_SHOWME = RegisterWindowMessage(CommonSharedConstants::WM_MAGPIE_SHOWME);
static const UINT WM_MAGPIE_QUIT = RegisterWindowMessage(CommonSharedConstants::WM_MAGPIE_QUIT);

// 提前加载 twinapi.appcore.dll 和 threadpoolwinrt.dll 以避免退出时崩溃。应在 Windows.UI.Xaml.dll 被加载前调用
// 来自 https://github.com/CommunityToolkit/Microsoft.Toolkit.Win32/blob/6fb2c3e00803ea563af20f6bc9363091b685d81f/Microsoft.Toolkit.Win32.UI.XamlApplication/XamlApplication.cpp#L140
// 参见：https://github.com/microsoft/microsoft-ui-xaml/issues/7260#issuecomment-1231314776
static void FixThreadPoolCrash() noexcept {
	LoadLibraryEx(L"twinapi.appcore.dll", nullptr, 0);
	LoadLibraryEx(L"threadpoolwinrt.dll", nullptr, 0);
}

bool XamlApp::Initialize(HINSTANCE hInstance, const wchar_t* arguments) {
	_hInst = hInstance;

	FixThreadPoolCrash();
	_InitializeLogger();

	Logger::Get().Info(fmt::format("程序启动\n\t版本：v{}.{}.{}\n\t管理员：{}",
		MAGPIE_VERSION.major, MAGPIE_VERSION.minor, MAGPIE_VERSION.patch,
		Win32Utils::IsProcessElevated() ? "是" : "否"));

	if (!_CheckSingleInstance()) {
		Logger::Get().Info("已经有一个实例正在运行");
		Logger::Get().Flush();
		return false;
	}

	// 初始化 UWP 应用
	_uwpApp = winrt::Magpie::App::App();

	winrt::Magpie::App::StartUpOptions options = _uwpApp.Initialize(0);
	if (options.IsError) {
		Logger::Get().Error("初始化失败");
		return false;
	}

	if (options.IsNeedElevated && !Win32Utils::IsProcessElevated()) {
		Restart(true, arguments);
		return false;
	}

	_mainWndRect = {
		(int)std::lroundf(options.MainWndRect.X),
		(int)std::lroundf(options.MainWndRect.Y),
		(int)std::lroundf(options.MainWndRect.Width),
		(int)std::lroundf(options.MainWndRect.Height)
	};
	_isMainWndMaximized = options.IsWndMaximized;

	ThemeHelper::Initialize();

	TrayIconService& trayIconService = TrayIconService::Get();
	trayIconService.Initialize();
	trayIconService.IsShow(_uwpApp.IsShowTrayIcon());
	_uwpApp.IsShowTrayIconChanged([](winrt::IInspectable const&, bool value) {
		TrayIconService::Get().IsShow(value);
	});

	_mainWindow.Destroyed({ this, &XamlApp::_MainWindow_Destoryed });

	// 不显示托盘图标时忽略 -t 参数
	if (!trayIconService.IsShow() || !arguments || arguments != L"-t"sv) {
		if (!_CreateMainWindow()) {
			Quit();
			return false;
		}
	}

	return true;
}

int XamlApp::Run() {
	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0)) {
		if (msg.message == WM_MAGPIE_SHOWME && _uwpApp) {
			ShowMainWindow();
			continue;
		} else if (msg.message == WM_MAGPIE_QUIT) {
			Quit();
			continue;
		}

		_mainWindow.HandleMessage(msg);
	}

	_ReleaseMutexes();

	return (int)msg.wParam;
}

void XamlApp::Quit() {
	if (_mainWindow) {
		_mainWindow.Destroy();
	}

	if (_uwpApp) {
		_QuitWithoutMainWindow();
	}
}

void XamlApp::Restart(bool asElevated, const wchar_t* arguments) noexcept {
	Quit();

	// 提前释放锁
	_ReleaseMutexes();

	SHELLEXECUTEINFO execInfo{};
	execInfo.cbSize = sizeof(execInfo);
	execInfo.lpFile = L"Magpie.exe";
	execInfo.lpParameters = arguments;
	execInfo.lpVerb = asElevated ? L"runas" : L"open";
	// 调用 ShellExecuteEx 后立即退出，因此应该指定 SEE_MASK_NOASYNC
	execInfo.fMask = SEE_MASK_NOASYNC;
	execInfo.nShow = SW_SHOWNORMAL;

	if (!ShellExecuteEx(&execInfo)) {
		Logger::Get().Win32Error("ShellExecuteEx 失败");
		Logger::Get().Flush();
	}
}

void XamlApp::SaveSettings() {
	if (_mainWindow && TrayIconService::Get().IsShow()) {
		WINDOWPLACEMENT wp{};
		wp.length = sizeof(wp);
		if (GetWindowPlacement(_mainWindow.Handle(), &wp)) {
			_mainWndRect = {
				wp.rcNormalPosition.left,
				wp.rcNormalPosition.top,
				wp.rcNormalPosition.right - wp.rcNormalPosition.left,
				wp.rcNormalPosition.bottom - wp.rcNormalPosition.top
			};
			_isMainWndMaximized = wp.showCmd == SW_MAXIMIZE;
		} else {
			Logger::Get().Win32Error("GetWindowPlacement 失败");
		}
	}

	_uwpApp.SaveSettings();
}

XamlApp::XamlApp() {}

XamlApp::~XamlApp() {}

bool XamlApp::_CheckSingleInstance() {
	static constexpr const wchar_t* SINGLE_INSTANCE_MUTEX_NAME = L"{4C416227-4A30-4A2F-8F23-8701544DD7D6}";
	static constexpr const wchar_t* ELEVATED_MUTEX_NAME = L"{E494C456-F587-4DAF-B68F-366278D31C45}";

	if (Win32Utils::IsProcessElevated()) {
		_hElevatedMutex.reset(CreateMutex(nullptr, TRUE, ELEVATED_MUTEX_NAME));
		if (!_hElevatedMutex || GetLastError() == ERROR_ALREADY_EXISTS) {
			// 通知已有实例显示主窗口
			PostMessage(HWND_BROADCAST, WM_MAGPIE_SHOWME, 0, 0);
			return false;
		}

		// 以管理员身份运行时允许接收 WM_MAGPIE_SHOWME 消息，否则无法被该消息唤醒
		if (!ChangeWindowMessageFilter(WM_MAGPIE_SHOWME, MSGFLT_ADD)) {
			Logger::Get().Win32Error("ChangeWindowMessageFilter 失败");
		}
	}

	_hSingleInstanceMutex.reset(CreateMutex(nullptr, TRUE, SINGLE_INSTANCE_MUTEX_NAME));
	if (!_hSingleInstanceMutex || GetLastError() == ERROR_ALREADY_EXISTS) {
		if (_hElevatedMutex) {
			if (!_hSingleInstanceMutex) {
				return false;
			}

			// 本实例是管理员身份，而已有实例不是。通知已有实例退出
			PostMessage(HWND_BROADCAST, WM_MAGPIE_QUIT, 0, 0);

			// 等待退出完成
			if (WaitForSingleObject(_hSingleInstanceMutex.get(), 10000) != WAIT_OBJECT_0) {
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

void XamlApp::_InitializeLogger() {
	Logger& logger = Logger::Get();
	logger.Initialize(
		spdlog::level::info,
		CommonSharedConstants::LOG_PATH,
		100000,
		2
	);

	// 初始化 dll 中的 Logger
	// Logger 的单例无法在 exe 和 dll 间共享
	winrt::Magpie::App::LoggerHelper::Initialize((uint64_t)&logger);
	Magpie::Core::LoggerHelper::Initialize(logger);
}

bool XamlApp::_CreateMainWindow() {
	if (!_mainWindow.Create(_hInst, _mainWndRect, _isMainWndMaximized)) {
		return false;
	}

	_uwpApp.HwndMain((uint64_t)_mainWindow.Handle());
	_uwpApp.MainPage(_mainWindow.Content());

	return true;
}

void XamlApp::ShowMainWindow() noexcept {
	if (_mainWindow) {
		_mainWindow.Show();
	} else {
		_CreateMainWindow();
	}
}

void XamlApp::_QuitWithoutMainWindow() {
	TrayIconService::Get().Uninitialize();

	_uwpApp.Uninitialize();
	// 不能调用 Close，否则切换页面时关闭主窗口会导致崩溃
	_uwpApp = nullptr;

	PostQuitMessage(0);

	Logger::Get().Info("程序退出");
	Logger::Get().Flush();
}

void XamlApp::_MainWindow_Destoryed() {
	_uwpApp.HwndMain(0);
	_uwpApp.MainPage(nullptr);

	if (!TrayIconService::Get().IsShow()) {
		_QuitWithoutMainWindow();
	}
}

void XamlApp::_ReleaseMutexes() noexcept {
	if (_hSingleInstanceMutex) {
		ReleaseMutex(_hSingleInstanceMutex.get());
		_hSingleInstanceMutex.reset();
	}
	if (_hElevatedMutex) {
		ReleaseMutex(_hElevatedMutex.get());
		_hElevatedMutex.reset();
	}
}

}
