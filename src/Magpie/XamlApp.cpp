#include "pch.h"
#include "XamlApp.h"
#include "Logger.h"
#include "StrUtils.h"
#include "Win32Utils.h"
#include "XamlUtils.h"
#include "CommonSharedConstants.h"
#include <CoreWindow.h>
#include <uxtheme.h>
#include <fmt/xchar.h>
#include <winrt/Magpie.Runtime.h>

#pragma comment(lib, "UxTheme.lib")


static constexpr const wchar_t* XAML_HOST_CLASS_NAME = L"Magpie_XamlHost";
static constexpr const wchar_t* MUTEX_NAME = L"{4C416227-4A30-4A2F-8F23-8701544DD7D6}";

static constexpr UINT CHECK_FORGROUND_TIMER_ID = 1;


bool XamlApp::Initialize(HINSTANCE hInstance) {
	_hMutex.reset(CreateMutex(nullptr, TRUE, MUTEX_NAME));
	if (!_hMutex || GetLastError() == ERROR_ALREADY_EXISTS) {
		// 将已存在的窗口带到前台
		// 可以唤醒旧版本，但旧版不能唤醒新版
		HWND hWnd = FindWindow(XAML_HOST_CLASS_NAME, nullptr);
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

	if (!_settings.Initialize()) {
		logger.Error("加载配置文件失败");
		MessageBox(NULL, L"加载配置文件失败", L"Magpie", MB_ICONERROR | MB_OK);
		return false;
	}

	// 注册窗口类
	{
		WNDCLASSEXW wcex{};

		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.lpfnWndProc = _WndProcStatic;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = hInstance;
		wcex.hIcon = NULL;
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.hbrBackground = NULL;
		wcex.lpszMenuName = NULL;
		wcex.lpszClassName = XAML_HOST_CLASS_NAME;
		wcex.hIconSm = NULL;

		RegisterClassEx(&wcex);
	}

	const winrt::Rect& windowRect = _settings.WindowRect();

	_hwndXamlHost = CreateWindow(
		XAML_HOST_CLASS_NAME,
		L"Magpie",
		WS_OVERLAPPEDWINDOW,
		(LONG)windowRect.X, (LONG)windowRect.Y, (LONG)windowRect.Width, (LONG)windowRect.Height,
		nullptr,
		nullptr,
		hInstance,
		nullptr
	);

	if (!_hwndXamlHost) {
		logger.Win32Error("CreateWindow 失败");
		return false;
	}

	bool isWin11 = Win32Utils::GetOSBuild() >= 22000;
	if (!isWin11) {
		// 此时 MainPage 尚未初始化
		_UpdateTheme();
	}

	// 防止窗口显示时背景闪烁
	// https://stackoverflow.com/questions/69715610/how-to-initialize-the-background-color-of-win32-app-to-something-other-than-whit
	SetWindowPos(_hwndXamlHost, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	if (isWin11) {
		// 标题栏不显示图标和标题，因为目前 DWM 存在 bug 无法在启用 Mica 时正确绘制标题
		WTA_OPTIONS option{};
		option.dwFlags = WTNCA_NODRAWCAPTION | WTNCA_NODRAWICON | WTNCA_NOSYSMENU;
		option.dwMask = WTNCA_VALIDBITS;
		SetWindowThemeAttribute(_hwndXamlHost, WTA_NONCLIENT, &option, sizeof(option));
	} else {
		// Win10 中任务栏可能出现空的 DesktopWindowXamlSource 窗口
		// 见 https://github.com/microsoft/microsoft-ui-xaml/issues/6490
		// 如果不将 ShowWindow 提前，任务栏会短暂出现两个图标
		ShowWindow(_hwndXamlHost, _settings.IsWindowMaximized() ? SW_MAXIMIZE : SW_SHOW);
	}

	// 初始化 UWP 应用和 XAML Islands
	_uwpApp = winrt::Magpie::App::App();
	if (!isWin11) {
		// 隐藏 DesktopWindowXamlSource 窗口
		winrt::CoreWindow coreWindow = winrt::CoreWindow::GetForCurrentThread();
		if (coreWindow) {
			HWND hwndDWXS;
			coreWindow.as<ICoreWindowInterop>()->get_WindowHandle(&hwndDWXS);
			ShowWindow(hwndDWXS, SW_HIDE);
		}
	}

	if (!_uwpApp.Initialize(_settings, (uint64_t)_hwndXamlHost)) {
		logger.Error("App 初始化失败");

		// 销毁主窗口
		DestroyWindow(_hwndXamlHost);
		MSG msg;
		while (GetMessage(&msg, nullptr, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		return false;
	}

	// 未显示窗口时视为位于前台，否则显示窗口的动画有小瑕疵
	_uwpApp.OnHostWndFocusChanged(true);

	_hotkeyManager = _uwpApp.HotkeyManager();

	_mainPage = winrt::Magpie::App::MainPage();
	_mainPage.ActualThemeChanged([this](winrt::FrameworkElement const&, winrt::IInspectable const&) {
		_UpdateTheme();
	});
	if (isWin11) {
		// Win11 中在 MainPage 加载完成后再显示主窗口
		_mainPage.Loaded([this](winrt::IInspectable const&, winrt::RoutedEventArgs const&) {
			ShowWindow(_hwndXamlHost, _settings.IsWindowMaximized() ? SW_MAXIMIZE : SW_SHOW);
			SetFocus(_hwndXamlHost);
		});
		_UpdateTheme();
	}

	_xamlSource = winrt::DesktopWindowXamlSource();
	_xamlSourceNative2 = _xamlSource.as<IDesktopWindowXamlSourceNative2>();

	auto interop = _xamlSource.as<IDesktopWindowXamlSourceNative>();
	interop->AttachToWindow(_hwndXamlHost);
	interop->get_WindowHandle(&_hwndXamlIsland);
	_xamlSource.Content(_mainPage);

	// 焦点始终位于 _hwndXamlIsland 中
	_xamlSource.TakeFocusRequested([](winrt::DesktopWindowXamlSource const& sender, winrt::DesktopWindowXamlSourceTakeFocusRequestedEventArgs const& args) {
		winrt::XamlSourceFocusNavigationReason reason = args.Request().Reason();
		if (reason < winrt::XamlSourceFocusNavigationReason::Left) {
			sender.NavigateFocus(args.Request());
		}
	});

	if (isWin11) {
		// SetTimer 之前推荐先调用 SetUserObjectInformation
		BOOL value = FALSE;
		if (!SetUserObjectInformation(GetCurrentProcess(), UOI_TIMERPROC_EXCEPTION_SUPPRESSION, &value, sizeof(value))) {
			logger.Win32Error("SetUserObjectInformation 失败");
		}

		// 监听 WM_ACTIVATE 不完全可靠，因此定期检查前台窗口以确保背景绘制正确
		if (SetTimer(_hwndXamlHost, CHECK_FORGROUND_TIMER_ID, 250, nullptr) == 0) {
			logger.Win32Error("SetTimer 失败");
		}
	} else {
		// Win10 中因为 ShowWindow 提前，需要显式设置 XAML Islands 位置
		_OnResize();
	}

	return true;
}

int XamlApp::Run() {
	MSG msg;

	// 主消息循环
	while (GetMessage(&msg, nullptr, 0, 0)) {
		BOOL processed = FALSE;
		HRESULT hr = _xamlSourceNative2->PreTranslateMessage(&msg, &processed);
		if (SUCCEEDED(hr) && processed) {
			continue;
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	_uwpApp.OnClose();

	_xamlSourceNative2 = nullptr;
	_xamlSource.Close();
	_xamlSource = nullptr;
	_mainPage = nullptr;
	_uwpApp = nullptr;

	Logger::Get().Info("程序退出");
	Logger::Get().Flush();

	return (int)msg.wParam;
}

void XamlApp::_OnResize() {
	RECT clientRect;
	GetClientRect(_hwndXamlHost, &clientRect);
	SetWindowPos(_hwndXamlIsland, NULL, 0, 0, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, SWP_SHOWWINDOW | SWP_NOACTIVATE);
}

void XamlApp::_UpdateTheme() {
	constexpr const DWORD DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 = 19;
	constexpr const DWORD DWMWA_MICA_EFFECT = 1029;

	BOOL isDarkTheme;
	if (_mainPage) {
		isDarkTheme = _mainPage.ActualTheme() == winrt::ElementTheme::Dark;
	} else {
		int theme = _settings.Theme();

		if (theme == 2) {
			isDarkTheme = winrt::UISettings().GetColorValue(winrt::UIColorType::Background).R < 128;
		} else {
			isDarkTheme = theme == 1;
		}
	}

	auto osBuild = Win32Utils::GetOSBuild();

	if (osBuild >= 22000) {
		// 在 Win11 中应用 Mica
		BOOL mica = TRUE;
		DwmSetWindowAttribute(_hwndXamlHost, DWMWA_MICA_EFFECT, &mica, sizeof(mica));
	}

	// 使标题栏适应黑暗模式
	// build 18985 之前 DWMWA_USE_IMMERSIVE_DARK_MODE 的值不同
	// https://github.com/MicrosoftDocs/sdk-api/pull/966/files
	DwmSetWindowAttribute(
		_hwndXamlHost,
		osBuild < 18985 ? DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 : DWMWA_USE_IMMERSIVE_DARK_MODE,
		& isDarkTheme,
		sizeof(isDarkTheme)
	);

	// 更改背景色以配合主题
	// 背景色在更改窗口大小时会短暂可见
	HBRUSH hbrOld = (HBRUSH)SetClassLongPtr(
		_hwndXamlHost,
		GCLP_HBRBACKGROUND,
		(INT_PTR)CreateSolidBrush(isDarkTheme ?
			CommonSharedConstants::DARK_TINT_COLOR : CommonSharedConstants::LIGHT_TINT_COLOR));
	if (hbrOld) {
		DeleteObject(hbrOld);
	}
	InvalidateRect(_hwndXamlHost, nullptr, TRUE);

	// 强制重绘标题栏
	LONG_PTR style = GetWindowLongPtr(_hwndXamlHost, GWL_EXSTYLE);
	if (osBuild < 22000) {
		// 在 Win10 上需要更多 hack
		SetWindowLongPtr(_hwndXamlHost, GWL_EXSTYLE, style | WS_EX_LAYERED);
		SetLayeredWindowAttributes(_hwndXamlHost, 0, 254, LWA_ALPHA);
	}
	SetWindowLongPtr(_hwndXamlHost, GWL_EXSTYLE, style);
}

// 使 ContentDialog 跟随窗口尺寸调整
void XamlApp::_ResizeXamlDialog() {
	winrt::XamlRoot root = _mainPage.XamlRoot();
	if (!root) {
		return;
	}

	winrt::Size rootSize = root.Size();

	for (const auto& popup : winrt::VisualTreeHelper::GetOpenPopupsForXamlRoot(root)) {
		if (!popup.IsConstrainedToRootBounds()) {
			continue;
		}

		winrt::FrameworkElement child = popup.Child().as<winrt::FrameworkElement>();
		child.Width(rootSize.Width);
		child.Height(rootSize.Height);
	}
}

void XamlApp::_RepositionXamlPopups() {
	winrt::XamlRoot root = _mainPage.XamlRoot();
	if (!root) {
		return;
	}

	for (const auto& popup : winrt::VisualTreeHelper::GetOpenPopupsForXamlRoot(root)) {
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
	case WM_HOTKEY:
	{
		using winrt::Magpie::App::HotkeyAction;
		if (wParam >= 0 && wParam < (UINT)HotkeyAction::COUNT_OR_NONE) {
			_hotkeyManager.OnHotkeyPressed((HotkeyAction)wParam);
			return 0;
		}

		break;
	}
	case WM_KEYDOWN:
	{
		if (wParam == VK_TAB) {
			// 处理焦点
			if (_xamlSource) {
				BYTE keyboardState[256] = {};
				if (GetKeyboardState(keyboardState)) {
					winrt::XamlSourceFocusNavigationReason reason = (keyboardState[VK_SHIFT] & 0x80) ?
						winrt::XamlSourceFocusNavigationReason::Last : winrt::XamlSourceFocusNavigationReason::First;
					_xamlSource.NavigateFocus(winrt::XamlSourceFocusNavigationRequest(reason));
				}
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
						app->_RepositionXamlPopups();
					});
				}(this);
			}
		}

		return 0;
	}
	case WM_WINDOWPOSCHANGED:
	{
		if (_xamlSource) {
			WINDOWPLACEMENT wp{};
			wp.length = sizeof(wp);
			if (GetWindowPlacement(_hwndXamlHost, &wp)) {
				_settings.IsWindowMaximized(wp.showCmd == SW_MAXIMIZE);

				RECT windowRect;
				if (GetWindowRect(_hwndXamlHost, &windowRect)) {
					_settings.WindowRect(winrt::Rect{
						(float)wp.rcNormalPosition.left,
						(float)wp.rcNormalPosition.top,
						float(wp.rcNormalPosition.right - wp.rcNormalPosition.left),
						float(wp.rcNormalPosition.bottom - wp.rcNormalPosition.top)
					});
				}
			}
		}

		// 交给 DefWindowProc 处理
		break;
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
			if (!IsWindowVisible(_hwndXamlHost) || GetForegroundWindow() == _hwndXamlHost) {
				_uwpApp.OnHostWndFocusChanged(true);
			} else {
				_uwpApp.OnHostWndFocusChanged(false);
				XamlUtils::CloseXamlPopups(_mainPage.XamlRoot());
			}
			_OnResize();
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
			_RepositionXamlPopups();
		}

		return 0;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

