#include "pch.h"
#include "XamlApp.h"
#include "Logger.h"
#include "StrUtils.h"
#include "Win32Utils.h"
#include "XamlUtils.h"
#include "CommonSharedConstants.h"
#include <uxtheme.h>
#include <fmt/xchar.h>
#include <winrt/Magpie.Runtime.h>
#include "resource.h"

#pragma comment(lib, "UxTheme.lib")


static constexpr const wchar_t* MUTEX_NAME = L"{4C416227-4A30-4A2F-8F23-8701544DD7D6}";

static constexpr UINT CHECK_FORGROUND_TIMER_ID = 1;

static constexpr const wchar_t* NOTIFY_ICON_WINDOW_CLASS_NAME = L"Magpie_NotifyIcon";


bool XamlApp::Initialize(HINSTANCE hInstance, const wchar_t* arguments) {
	_hInst = hInstance;

	_hMutex.reset(CreateMutex(nullptr, TRUE, MUTEX_NAME));
	if (!_hMutex || GetLastError() == ERROR_ALREADY_EXISTS) {
		// 将已存在的窗口带到前台
		// 可以唤醒旧版本，但旧版不能唤醒新版
		HWND hWnd = FindWindow(CommonSharedConstants::XAML_HOST_CLASS_NAME, nullptr);
		if (hWnd) {
			// 如果已有实例权限更高 ShowWindow 会失败
			ShowWindow(hWnd, SW_NORMAL);
			Win32Utils::SetForegroundWindow(hWnd);
		} else {
			// 唤醒旧版本
			PostMessage(HWND_BROADCAST, RegisterWindowMessage(L"WM_SHOWME"), 0, 0);
		}

		return false;
	}

	// 当前目录始终是程序所在目录
	{
		wchar_t curDir[MAX_PATH] = { 0 };
		GetModuleFileName(NULL, curDir, MAX_PATH);

		for (int i = lstrlenW(curDir) - 1; i >= 0; --i) {
			if (curDir[i] == L'\\' || curDir[i] == L'/') {
				break;
			} else {
				curDir[i] = L'\0';
			}
		}

		SetCurrentDirectory(curDir);
	}

	Logger& logger = Logger::Get();
	logger.Initialize(
		spdlog::level::info,
		CommonSharedConstants::LOG_PATH,
		100000,
		2
	);

	logger.Info(fmt::format("程序启动\n\t版本：{}", MAGPIE_VERSION));

	// 初始化 dll 中的 Logger
	// Logger 的单例无法在 exe 和 dll 间共享
	winrt::Magpie::App::LoggerHelper::Initialize((uint64_t)&logger);
	winrt::Magpie::Runtime::LoggerHelper::Initialize((uint64_t)&logger);

	// 初始化 UWP 应用
	_uwpApp = winrt::Magpie::App::App();

	winrt::Magpie::App::StartUpOptions options = _uwpApp.Initialize(0);
	if (options.IsError) {
		Logger::Get().Error("初始化失败");
		return false;
	}

	if (options.IsNeedElevated && !Win32Utils::IsProcessElevated()) {
		_RestartAsElevated(arguments);
		return true;
	}

	_mainWndRect = {
		(int)std::lroundf(options.MainWndRect.X),
		(int)std::lroundf(options.MainWndRect.Y),
		(int)std::lroundf(options.MainWndRect.Width),
		(int)std::lroundf(options.MainWndRect.Height)
	};
	_isMainWndMaximized = options.IsWndMaximized;

	// 注册窗口类
	{
		WNDCLASSEXW wcex{};
		wcex.cbSize = sizeof(wcex);
		wcex.lpfnWndProc = _WndProcStatic;
		wcex.hInstance = hInstance;
		wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP));
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.lpszClassName = CommonSharedConstants::XAML_HOST_CLASS_NAME;

		RegisterClassEx(&wcex);
	}
	{
		WNDCLASSEXW wcex{};
		wcex.cbSize = sizeof(wcex);
		wcex.hInstance = hInstance;
		wcex.lpfnWndProc = _TrayIconWndProcStatic;
		wcex.lpszClassName = NOTIFY_ICON_WINDOW_CLASS_NAME;

		RegisterClassEx(&wcex);
	}

	_nid.cbSize = sizeof(_nid);
	_nid.uVersion = 0;	// 不使用 NOTIFYICON_VERSION_4
	_nid.uCallbackMessage = CommonSharedConstants::WM_NOTIFY_ICON;
	_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	_nid.uID = 0;

	// SetTimer 之前推荐先调用 SetUserObjectInformation
	BOOL value = FALSE;
	if (!SetUserObjectInformation(GetCurrentProcess(), UOI_TIMERPROC_EXCEPTION_SUPPRESSION, &value, sizeof(value))) {
		logger.Win32Error("SetUserObjectInformation 失败");
	}

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
	
	_uwpApp.SaveSettings();
	_uwpApp = nullptr;

	_HideTrayIcon();

	Logger::Get().Info("程序退出");
	Logger::Get().Flush();

	return (int)msg.wParam;
}

void XamlApp::_CreateMainWindow() {
	_hwndMain = CreateWindow(
		CommonSharedConstants::XAML_HOST_CLASS_NAME,
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

	if (Win32Utils::GetOSBuild() >= 22000) {
		// 标题栏不显示图标和标题，因为目前 DWM 存在 bug 无法在启用 Mica 时正确绘制标题
		WTA_OPTIONS option{};
		option.dwFlags = WTNCA_NODRAWCAPTION | WTNCA_NODRAWICON | WTNCA_NOSYSMENU;
		option.dwMask = WTNCA_VALIDBITS;
		SetWindowThemeAttribute(_hwndMain, WTA_NONCLIENT, &option, sizeof(option));

		// 监听 WM_ACTIVATE 不完全可靠，因此定期检查前台窗口以确保背景绘制正确
		if (SetTimer(_hwndMain, CHECK_FORGROUND_TIMER_ID, 250, nullptr) == 0) {
			Logger::Get().Win32Error("SetTimer 失败");
		}
	}

	_uwpApp.HwndMain((uint64_t)_hwndMain);
	// 未显示窗口时视为位于前台，否则显示窗口的动画有小瑕疵
	_uwpApp.OnHostWndFocusChanged(true);

	_mainPage = winrt::Magpie::App::MainPage();
	_uwpApp.MainPage(_mainPage);

	_mainPage.ActualThemeChanged([this](winrt::FrameworkElement const&, winrt::IInspectable const&) {
		_UpdateTheme();
	});
	_UpdateTheme();

	// MainPage 加载完成后显示主窗口
	_mainPage.Loaded([this](winrt::IInspectable const&, winrt::RoutedEventArgs const&)->winrt::IAsyncAction {
		co_await _mainPage.Dispatcher().RunAsync(winrt::CoreDispatcherPriority::Normal, [this]() {
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

	_OnResize();

	// 焦点始终位于 _hwndXamlIsland 中
	_xamlSource.TakeFocusRequested([](winrt::DesktopWindowXamlSource const& sender, winrt::DesktopWindowXamlSourceTakeFocusRequestedEventArgs const& args) {
		winrt::XamlSourceFocusNavigationReason reason = args.Request().Reason();
		if (reason < winrt::XamlSourceFocusNavigationReason::Left) {
			sender.NavigateFocus(args.Request());
		}
	});
}

void XamlApp::_ShowMainWindow() noexcept {
	if (_hwndMain) {
		if (IsIconic(_hwndMain)) {
			ShowWindow(_hwndMain, SW_RESTORE);
		}

		SetForegroundWindow(_hwndMain);
	} else {
		_CreateMainWindow();
	}
}

void XamlApp::_Quit() noexcept {
	_HideTrayIcon();

	if (_hwndMain) {
		DestroyWindow(_hwndMain);
	}

	PostQuitMessage(0);
}

void XamlApp::_RestartAsElevated(const wchar_t* arguments) noexcept {
	if (_hwndMain) {
		DestroyWindow(_hwndMain);
	}

	// 提前释放锁
	_hMutex.reset();

	wchar_t exePath[MAX_PATH]{};
	GetModuleFileName(NULL, exePath, MAX_PATH);

	SHELLEXECUTEINFOW execInfo{};
	execInfo.cbSize = sizeof(execInfo);
	execInfo.lpFile = exePath;
	execInfo.lpParameters = arguments;
	execInfo.lpVerb = L"runas";
	// 调用 ShellExecuteEx 后立即退出，因此应该指定 SEE_MASK_NOASYNC
	execInfo.fMask = SEE_MASK_NOASYNC;
	execInfo.nShow = SW_SHOWDEFAULT;

	if (!ShellExecuteEx(&execInfo)) {
		Logger::Get().Win32Error("ShellExecuteEx 失败");
	}

	PostQuitMessage(0);
}

void XamlApp::_ShowTrayIcon() noexcept {
	if (_nid.hWnd) {
		return;
	}

	// 创建一个隐藏的、message-only 的窗口用于接收托盘图标消息
	_nid.hWnd = CreateWindow(NOTIFY_ICON_WINDOW_CLASS_NAME, nullptr, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, _hInst, 0);
	LoadIconMetric(_hInst, MAKEINTRESOURCE(IDI_APP), LIM_SMALL, &_nid.hIcon);
	wcscpy_s(_nid.szTip, std::size(_nid.szTip), L"Magpie");

	if (!Shell_NotifyIcon(NIM_ADD, &_nid)) {
		// 创建托盘图标失败，可能是因为已经存在
		Shell_NotifyIcon(NIM_DELETE, &_nid);
		if (!Shell_NotifyIcon(NIM_ADD, &_nid)) {
			Logger::Get().Win32Error("创建托盘图标失败");
			_HideTrayIcon();
			return;
		}
	}
}

void XamlApp::_HideTrayIcon() noexcept {
	if (!_nid.hWnd) {
		return;
	}

	Shell_NotifyIcon(NIM_DELETE, &_nid);
	DestroyIcon(_nid.hIcon);
	_nid.hIcon = NULL;

	DestroyWindow(_nid.hWnd);
	_nid.hWnd = NULL;
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
	constexpr const DWORD DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 = 19;
	constexpr const DWORD DWMWA_MICA_EFFECT = 1029;

	auto osBuild = Win32Utils::GetOSBuild();

	if (osBuild >= 22000) {
		// 在 Win11 中应用 Mica
		BOOL mica = TRUE;
		DwmSetWindowAttribute(_hwndMain, DWMWA_MICA_EFFECT, &mica, sizeof(mica));
	}

	BOOL isDarkTheme = _mainPage.ActualTheme() == winrt::ElementTheme::Dark;

	// 使标题栏适应黑暗模式
	// build 18985 之前 DWMWA_USE_IMMERSIVE_DARK_MODE 的值不同
	// https://github.com/MicrosoftDocs/sdk-api/pull/966/files
	DwmSetWindowAttribute(
		_hwndMain,
		osBuild < 18985 ? DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 : DWMWA_USE_IMMERSIVE_DARK_MODE,
		&isDarkTheme,
		sizeof(isDarkTheme)
	);

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

	// 强制重绘标题栏
	LONG_PTR style = GetWindowLongPtr(_hwndMain, GWL_EXSTYLE);
	if (osBuild < 22000) {
		// 在 Win10 上需要更多 hack
		SetWindowLongPtr(_hwndMain, GWL_EXSTYLE, style | WS_EX_LAYERED);
		SetLayeredWindowAttributes(_hwndMain, 0, 254, LWA_ALPHA);
	}
	SetWindowLongPtr(_hwndMain, GWL_EXSTYLE, style);
}

// 使 ContentDialog 跟随窗口尺寸调整
void XamlApp::_ResizeXamlDialog() {
	winrt::XamlRoot root = _mainPage.XamlRoot();
	if (!root) {
		return;
	}

	winrt::Size rootSize = root.Size();

	for (const auto& popup : winrt::VisualTreeHelper::GetOpenPopupsForXamlRoot(root)) {
		winrt::UIElement child = popup.Child();
		winrt::hstring className = winrt::get_class_name(child);
		if (className == winrt::name_of<winrt::Controls::ContentDialog>() || className == winrt::name_of<winrt::Shapes::Rectangle>()) {
			winrt::FrameworkElement fe = child.as<winrt::FrameworkElement>();
			fe.Width(rootSize.Width);
			fe.Height(rootSize.Height);
		}
	}
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

LRESULT XamlApp::_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
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
				[](XamlApp* app)->winrt::fire_and_forget {
					co_await app->_mainPage.Dispatcher().RunAsync(winrt::CoreDispatcherPriority::Normal, [app]() {
						app->_ResizeXamlDialog();
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
				_uwpApp.OnHostWndFocusChanged(true);
				SetFocus(_hwndXamlIsland);
			} else {
				_uwpApp.OnHostWndFocusChanged(false);
				XamlUtils::CloseXamlPopups(_mainPage.XamlRoot());
			}
		}
		
		return 0;
	}
	case WM_TIMER:
	{
		if (wParam == CHECK_FORGROUND_TIMER_ID) {
			if (!IsWindowVisible(_hwndMain) || GetForegroundWindow() == _hwndMain) {
				_uwpApp.OnHostWndFocusChanged(true);
			} else {
				_uwpApp.OnHostWndFocusChanged(false);
				XamlUtils::CloseXamlPopups(_mainPage.XamlRoot());
			}
			return 0;
		}

		break;
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
		if (_nid.hWnd) {
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

		if (!_nid.hWnd) {
			PostQuitMessage(0);
		}

		return 0;
	}
	case CommonSharedConstants::WM_RESTART_AS_ELEVATED:
	{
		_RestartAsElevated();
		return 0;
	}
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
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
			HMENU hMenu = CreatePopupMenu();
			AppendMenu(hMenu, MF_STRING, 1, L"主窗口");
			AppendMenu(hMenu, MF_STRING, 2, L"退出");

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
				_Quit();
				break;
			}
			}
			break;
		}
		}

		return 0;
	}
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}
