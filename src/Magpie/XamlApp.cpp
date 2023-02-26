#include "pch.h"
#include "XamlApp.h"
#include "Logger.h"
#include "Win32Utils.h"
#include "XamlUtils.h"
#include "CommonSharedConstants.h"
#include <fmt/xchar.h>
#include "resource.h"
#include <Magpie.Core.h>
#include "Version.h"
#include "ThemeHelper.h"
#include <winrt/Windows.UI.WindowManagement.h>
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>
#include <CoreWindow.h>

namespace Magpie {

static const UINT WM_MAGPIE_SHOWME = RegisterWindowMessage(CommonSharedConstants::WM_MAGPIE_SHOWME);
// 当任务栏被创建时会广播此消息。用于在资源管理器被重新启动时重新创建托盘图标
// https://learn.microsoft.com/en-us/windows/win32/shell/taskbar#taskbar-creation-notification
static const UINT WM_TASKBARCREATED = RegisterWindowMessage(L"TaskbarCreated");

// https://github.com/microsoft/microsoft-ui-xaml/issues/7260#issuecomment-1231314776
// 提前加载 threadpoolwinrt.dll 以避免退出时崩溃。应在 Windows.UI.Xaml.dll 被加载前调用
static void FixThreadPoolCrash() noexcept {
	LoadLibraryEx(L"threadpoolwinrt.dll", nullptr, 0);
}

bool XamlApp::Initialize(HINSTANCE hInstance, const wchar_t* arguments) {
	_hInst = hInstance;

	if (!_CheckSingleInstance()) {
		return false;
	}

	FixThreadPoolCrash();
	_InitializeLogger();

	Logger::Get().Info(fmt::format("程序启动\n\t版本：{}\n\t管理员：{}",
		MAGPIE_TAG, Win32Utils::IsProcessElevated() ? "是" : "否"));

	// 初始化 UWP 应用
	_uwpApp = winrt::Magpie::App::App();

	winrt::Magpie::App::StartUpOptions options = _uwpApp.Initialize(0);
	if (options.IsError) {
		Logger::Get().Error("初始化失败");
		return false;
	}

	if (options.IsNeedElevated && !Win32Utils::IsProcessElevated()) {
		_Restart(true, arguments);
		return true;
	}

	_mainWndRect = {
		(int)std::lroundf(options.MainWndRect.X),
		(int)std::lroundf(options.MainWndRect.Y),
		(int)std::lroundf(options.MainWndRect.Width),
		(int)std::lroundf(options.MainWndRect.Height)
	};
	_isMainWndMaximized = options.IsWndMaximized;

	ThemeHelper::Initialize();

	// 注册窗口类
	{
		WNDCLASSEXW wcex{};
		wcex.cbSize = sizeof(wcex);
		wcex.lpfnWndProc = _WndProcStatic;
		wcex.hInstance = hInstance;
		wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP));
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.lpszClassName = CommonSharedConstants::MAIN_WINDOW_CLASS_NAME;

		RegisterClassEx(&wcex);
	}
	{
		WNDCLASSEXW wcex{};
		wcex.cbSize = sizeof(wcex);
		wcex.hInstance = hInstance;
		wcex.lpfnWndProc = _TrayIconWndProcStatic;
		wcex.lpszClassName = CommonSharedConstants::NOTIFY_ICON_WINDOW_CLASS_NAME;

		RegisterClassEx(&wcex);
	}

	_nid.cbSize = sizeof(_nid);
	_nid.uVersion = 0;	// 不使用 NOTIFYICON_VERSION_4
	_nid.uCallbackMessage = CommonSharedConstants::WM_NOTIFY_ICON;
	_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	_nid.uID = 0;

	bool isShowTrayIcon = _uwpApp.IsShowTrayIcon();
	if (isShowTrayIcon) {
		_ShowTrayIcon();
	}

	// 不常驻后台时忽略 -t 参数
	if (!isShowTrayIcon || !arguments || arguments != L"-t"sv) {
		_CreateMainWindow();
	}

	_uwpApp.IsShowTrayIconChanged([this](winrt::IInspectable const&, bool value) {
		if (value) {
			_ShowTrayIcon();
		} else {
			_HideTrayIcon();
		}
	});

	return true;
}

int XamlApp::Run() {
	MSG msg;

	// 主消息循环
	while (GetMessage(&msg, nullptr, 0, 0)) {
		if (msg.message == WM_MAGPIE_SHOWME) {
			_ShowMainWindow();
			continue;
		}

		if (_xamlSourceNative2) {
			BOOL processed = FALSE;
			HRESULT hr = _xamlSourceNative2->PreTranslateMessage(&msg, &processed);
			if (SUCCEEDED(hr) && processed) {
				continue;
			}
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}

XamlApp::XamlApp() {}

XamlApp::~XamlApp() {}

bool XamlApp::_CheckSingleInstance() {
	static constexpr const wchar_t* SINGLE_INSTANCE_MUTEX_NAME = L"{4C416227-4A30-4A2F-8F23-8701544DD7D6}";

	_hSingleInstanceMutex.reset(CreateMutex(nullptr, TRUE, SINGLE_INSTANCE_MUTEX_NAME));
	if (!_hSingleInstanceMutex || GetLastError() == ERROR_ALREADY_EXISTS) {
		// 通知已有实例显示主窗口
		PostMessage(HWND_BROADCAST, WM_MAGPIE_SHOWME, 0, 0);
		return false;
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

void XamlApp::_CreateMainWindow() {
	// Win11 22H2 中为了使用 Mica 背景需指定 WS_EX_NOREDIRECTIONBITMAP
	_hwndMain = CreateWindowEx(
		Win32Utils::GetOSVersion().Is22H2OrNewer() ? WS_EX_NOREDIRECTIONBITMAP : 0,
		CommonSharedConstants::MAIN_WINDOW_CLASS_NAME,
		L"Magpie",
		WS_OVERLAPPEDWINDOW,
		_mainWndRect.left, _mainWndRect.top, _mainWndRect.right, _mainWndRect.bottom,
		nullptr,
		nullptr,
		_hInst,
		nullptr
	);

	if (!_hwndMain) {
		Logger::Get().Win32Error("CreateWindow 失败");
		return;
	}

	_uwpApp.HwndMain((uint64_t)_hwndMain);

	_mainPage = winrt::Magpie::App::MainPage();
	_mainPage.ActualThemeChanged([this](winrt::FrameworkElement const&, winrt::IInspectable const&) {
		_UpdateTheme();
	});
	_UpdateTheme();

	// MainPage 加载完成后显示主窗口
	_mainPage.Loaded([this](winrt::IInspectable const&, winrt::RoutedEventArgs const&) -> winrt::fire_and_forget {
		co_await _mainPage.Dispatcher().TryRunAsync(winrt::CoreDispatcherPriority::Normal, [this]() {
			// 防止窗口显示时背景闪烁
			// https://stackoverflow.com/questions/69715610/how-to-initialize-the-background-color-of-win32-app-to-something-other-than-whit
			SetWindowPos(_hwndMain, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			ShowWindow(_hwndMain, _isMainWndMaximized ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL);
			SetForegroundWindow(_hwndMain);
		});
	});

	// 初始化 XAML Islands
	_xamlSource = winrt::DesktopWindowXamlSource();
	_xamlSourceNative2 = _xamlSource.as<IDesktopWindowXamlSourceNative2>();

	auto interop = _xamlSource.as<IDesktopWindowXamlSourceNative>();
	interop->AttachToWindow(_hwndMain);
	interop->get_WindowHandle(&_hwndXamlIsland);
	_xamlSource.Content(_mainPage);

	// 防止第一次收到 WM_SIZE 消息时 MainPage 尺寸为 0
	_OnResize();

	// 焦点始终位于 _hwndXamlIsland 中
	_xamlSource.TakeFocusRequested([](winrt::DesktopWindowXamlSource const& sender, winrt::DesktopWindowXamlSourceTakeFocusRequestedEventArgs const& args) {
		winrt::XamlSourceFocusNavigationReason reason = args.Request().Reason();
		if (reason < winrt::XamlSourceFocusNavigationReason::Left) {
			sender.NavigateFocus(args.Request());
		}
	});

	_uwpApp.MainPage(_mainPage);
}

void XamlApp::_ShowMainWindow() noexcept {
	if (_hwndMain) {
		if (IsIconic(_hwndMain)) {
			ShowWindow(_hwndMain, SW_RESTORE);
		}

		Win32Utils::SetForegroundWindow(_hwndMain);
	} else {
		_CreateMainWindow();
	}
}

void XamlApp::_Quit() noexcept {
	if (_hwndMain) {
		const bool isShowTrayIcon = _uwpApp.IsShowTrayIcon();

		DestroyWindow(_hwndMain);

		if (!isShowTrayIcon) {
			// WM_DESTROY 中已调用 _Quit()
			return;
		}
	}

	_HideTrayIcon();

	if (_nid.hWnd) {
		DestroyWindow(_nid.hWnd);
		_nid.hWnd = NULL;
	}
	if (_nid.hIcon) {
		DestroyIcon(_nid.hIcon);
		_nid.hIcon = NULL;
	}

	_uwpApp.Uninitialize();
	// 不能调用 Close，否则切换页面时关闭主窗口会导致崩溃
	_uwpApp = nullptr;
	PostQuitMessage(0);

	Logger::Get().Info("程序退出");
	Logger::Get().Flush();
}

void XamlApp::_Restart(bool asElevated, const wchar_t* arguments) noexcept {
	_Quit();

	// 提前释放锁
	_hSingleInstanceMutex.reset();

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
	}
}

void XamlApp::_ShowTrayIcon() noexcept {
	if (!_nid.hWnd) {
		// 创建一个隐藏窗口用于接收托盘图标消息
		_nid.hWnd = CreateWindow(
			CommonSharedConstants::NOTIFY_ICON_WINDOW_CLASS_NAME,
			nullptr,
			WS_OVERLAPPEDWINDOW | WS_POPUP,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			NULL,
			NULL,
			_hInst,
			0
		);

		LoadIconMetric(_hInst, MAKEINTRESOURCE(IDI_APP), LIM_SMALL, &_nid.hIcon);
		wcscpy_s(_nid.szTip, std::size(_nid.szTip), L"Magpie");
	}

	if (!Shell_NotifyIcon(NIM_ADD, &_nid)) {
		// 创建托盘图标失败，可能是因为已经存在
		Shell_NotifyIcon(NIM_DELETE, &_nid);
		if (!Shell_NotifyIcon(NIM_ADD, &_nid)) {
			Logger::Get().Win32Error("创建托盘图标失败");
			_isTrayIconCreated = false;
			return;
		}
	}

	_isTrayIconCreated = true;
}

void XamlApp::_HideTrayIcon() noexcept {
	Shell_NotifyIcon(NIM_DELETE, &_nid);
	_isTrayIconCreated = false;
}

void XamlApp::_OnResize() {
	if (!_hwndMain || !_hwndXamlIsland) {
		return;
	}

	RECT clientRect;
	GetClientRect(_hwndMain, &clientRect);
	SetWindowPos(_hwndXamlIsland, NULL, 0, 0, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, SWP_SHOWWINDOW | SWP_NOACTIVATE);
}

void XamlApp::_UpdateTheme() {
	const bool isDarkTheme = _mainPage.ActualTheme() == winrt::ElementTheme::Dark;

	if (Win32Utils::GetOSVersion().Is22H2OrNewer()) {
		// 设置 Mica 背景
		DWM_SYSTEMBACKDROP_TYPE value = DWMSBT_MAINWINDOW;
		DwmSetWindowAttribute(_hwndMain, DWMWA_SYSTEMBACKDROP_TYPE, &value, sizeof(value));
	} else {
		// 更改背景色以配合主题
		// 背景色在更改窗口大小时会短暂可见
		HBRUSH hbrOld = (HBRUSH)SetClassLongPtr(
			_hwndMain,
			GCLP_HBRBACKGROUND,
			(INT_PTR)CreateSolidBrush(isDarkTheme ?
			CommonSharedConstants::DARK_TINT_COLOR : CommonSharedConstants::LIGHT_TINT_COLOR));
		if (hbrOld) {
			DeleteObject(hbrOld);
		}
		InvalidateRect(_hwndMain, nullptr, TRUE);
	}

	ThemeHelper::SetWindowTheme(_hwndMain, isDarkTheme);
}

void XamlApp::_RepositionXamlPopups(bool closeFlyoutPresenter) {
	winrt::XamlRoot root = _mainPage.XamlRoot();
	if (!root) {
		return;
	}

	for (const auto& popup : winrt::VisualTreeHelper::GetOpenPopupsForXamlRoot(root)) {
		if (closeFlyoutPresenter) {
			auto className = winrt::get_class_name(popup.Child());
			if (className == winrt::name_of<winrt::Controls::FlyoutPresenter>() ||
				className == winrt::name_of<winrt::Controls::MenuFlyoutPresenter>()
			) {
				popup.IsOpen(false);
				continue;
			}
		}

		// 取自 https://github.com/CommunityToolkit/Microsoft.Toolkit.Win32/blob/229fa3cd245ff002906b2a594196b88aded25774/Microsoft.Toolkit.Forms.UI.XamlHost/WindowsXamlHostBase.cs#L180

		// Toggle the CompositeMode property, which will force all windowed Popups
		// to reposition themselves relative to the new position of the host window.
		auto compositeMode = popup.CompositeMode();

		// Set CompositeMode to some value it currently isn't set to.
		if (compositeMode == winrt::ElementCompositeMode::SourceOver) {
			popup.CompositeMode(winrt::ElementCompositeMode::MinBlend);
		} else {
			popup.CompositeMode(winrt::ElementCompositeMode::SourceOver);
		}

		// Restore CompositeMode to whatever it was originally set to.
		popup.CompositeMode(compositeMode);
	}
}

LRESULT XamlApp::_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_SHOWWINDOW:
	{
		if (wParam == TRUE) {
			SetFocus(_hwndMain);
		}

		break;
	}
	case WM_KEYDOWN:
	{
		if (wParam == VK_TAB) {
			// 处理焦点
			if (_xamlSource) {
				winrt::XamlSourceFocusNavigationReason reason = (GetKeyState(VK_SHIFT) & 0x80) ?
					winrt::XamlSourceFocusNavigationReason::Last : winrt::XamlSourceFocusNavigationReason::First;
				_xamlSource.NavigateFocus(winrt::XamlSourceFocusNavigationRequest(reason));
			}
			return 0;
		}
	}
	case WM_SIZE:
	{
		if (wParam != SIZE_MINIMIZED) {
			_OnResize();
			if (_mainPage) {
				// 使 ContentDialog 跟随窗口尺寸调整
				// 来自 https://github.com/microsoft/microsoft-ui-xaml/issues/3577#issuecomment-1399250405
				if (winrt::CoreWindow coreWindow = winrt::CoreWindow::GetForCurrentThread()) {
					HWND hwndDWXS;
					coreWindow.as<ICoreWindowInterop>()->get_WindowHandle(&hwndDWXS);
					PostMessage(hwndDWXS, WM_SIZE, wParam, lParam);
				}

				[](XamlApp* app) -> winrt::fire_and_forget {
					co_await app->_mainPage.Dispatcher().TryRunAsync(winrt::CoreDispatcherPriority::Normal, [app]() {
						app->_RepositionXamlPopups(true);
					});
				}(this);
			}
		}

		return 0;
	}
	case WM_GETMINMAXINFO:
	{
		// 设置窗口最小尺寸
		MINMAXINFO* mmi = (MINMAXINFO*)lParam;
		mmi->ptMinTrackSize = { 500,300 };
		return 0;
	}
	case WM_ACTIVATE:
	{
		if (_uwpApp) {
			if (LOWORD(wParam) != WA_INACTIVE) {
				SetFocus(_hwndXamlIsland);
			} else {
				XamlUtils::CloseXamlPopups(_mainPage.XamlRoot());
			}
		}

		return 0;
	}
	case WM_SYSCOMMAND:
	{
		// Alt 键默认会打开菜单，导致界面不响应鼠标移动。这里禁用这个行为
		if ((wParam & 0xfff0) == SC_KEYMENU) {
			return 0;
		}

		// 最小化时关闭 ComboBox
		// 不能在 WM_SIZE 中处理，该消息发送于最小化之后，会导致 ComboBox 无法交互
		if (wParam == SC_MINIMIZE && _mainPage) {
			XamlUtils::CloseXamlPopups(_mainPage.XamlRoot());
		}

		break;
	}
	case WM_MOVING:
	{
		if (_mainPage) {
			_RepositionXamlPopups(false);
		}

		return 0;
	}
	case WM_DPICHANGED:
	{
		RECT* newRect = (RECT*)lParam;
		SetWindowPos(hWnd,
			NULL,
			newRect->left,
			newRect->top,
			newRect->right - newRect->left,
			newRect->bottom - newRect->top,
			SWP_NOZORDER | SWP_NOACTIVATE
		);

		break;
	}
	case WM_DESTROY:
	{
		const bool isShowTrayIcon = _uwpApp.IsShowTrayIcon();
		if (isShowTrayIcon) {
			WINDOWPLACEMENT wp{};
			wp.length = sizeof(wp);
			if (GetWindowPlacement(_hwndMain, &wp)) {
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
		_uwpApp.HwndMain(0);
		_uwpApp.MainPage(nullptr);

		_hwndMain = NULL;
		_xamlSourceNative2 = nullptr;
		// 必须手动重置 Content，否则会内存泄露，使 MainPage 无法析构
		_xamlSource.Content(nullptr);
		_xamlSource.Close();
		_xamlSource = nullptr;

		_mainPage = nullptr;

		if (!isShowTrayIcon) {
			_Quit();
		}

		return 0;
	}
	case CommonSharedConstants::WM_QUIT_MAGPIE:
	{
		_Quit();
		return 0;
	}
	case CommonSharedConstants::WM_RESTART_MAGPIE:
	{
		_Restart(false);
		return 0;
	}
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT XamlApp::_TrayIconWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case CommonSharedConstants::WM_NOTIFY_ICON:
	{
		switch (lParam) {
		case WM_LBUTTONDBLCLK:
		{
			_ShowMainWindow();
			break;
		}
		case WM_RBUTTONUP:
		{
			winrt::ResourceLoader resourceLoader = winrt::ResourceLoader::GetForCurrentView();
			winrt::hstring mainWindowText = resourceLoader.GetString(L"TrayIcon_MainWindow");
			winrt::hstring exitText = resourceLoader.GetString(L"TrayIcon_Exit");

			HMENU hMenu = CreatePopupMenu();
			AppendMenu(hMenu, MF_STRING, 1, mainWindowText.c_str());
			AppendMenu(hMenu, MF_STRING, 2, exitText.c_str());

			// hWnd 必须为前台窗口才能正确展示弹出菜单
			// 即使 hWnd 是隐藏的
			SetForegroundWindow(hWnd);

			POINT cursorPos;
			GetCursorPos(&cursorPos);
			BOOL selectedMenuId = TrackPopupMenuEx(hMenu, TPM_LEFTALIGN | TPM_NONOTIFY | TPM_RETURNCMD, cursorPos.x, cursorPos.y, hWnd, nullptr);

			DestroyMenu(hMenu);

			switch (selectedMenuId) {
			case 1:
			{
				_ShowMainWindow();
				break;
			}
			case 2:
			{
				if (!_hwndMain) {
					_uwpApp.SaveSettings();
				}
				
				_Quit();
				break;
			}
			}
			break;
		}
		}

		return 0;
	}
	case WM_WINDOWPOSCHANGING:
	{
		// 如果 Magpie 启动时任务栏尚未被创建，Shell_NotifyIcon 会失败，因此无法收到 WM_TASKBARCREATED 消息。
		// 监听 WM_WINDOWPOSCHANGING 以在资源管理器启动时获得通知
		// hack 来自 https://github.com/microsoft/PowerToys/pull/789
		if (!_isTrayIconCreated && _uwpApp.IsShowTrayIcon()) {
			_ShowTrayIcon();
		}
		break;
	}
	default:
	{
		if (message == WM_TASKBARCREATED) {
			if (_uwpApp.IsShowTrayIcon()) {
				// 重新创建任务栏图标
				_ShowTrayIcon();
			}
		}
	}
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

}
