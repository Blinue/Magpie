#include "pch.h"
#include "MainWindow.h"
#include "App.h"
#include "AppSettings.h"
#include "CaptionButtonsControl.h"
#include "CommonSharedConstants.h"
#include "resource.h"
#include "SmoothResizeHelper.h"
#include "ThemeHelper.h"
#include "TitleBarControl.h"
#include "Win32Helper.h"
#include "XamlHelper.h"
#include <CoreWindow.h>
#include <ShellScalingApi.h>

using namespace winrt::Magpie::implementation;
namespace winrt {
using namespace Windows::UI::Xaml::Hosting;
}

namespace Magpie {

MainWindow::~MainWindow() noexcept {
	Destroy();
}

bool MainWindow::Create() noexcept {
	[[maybe_unused]] static Ignore _ = [] {
		const HINSTANCE hInstance = wil::GetModuleInstanceHandle();

		WNDCLASSEXW wcex = {
			.cbSize = sizeof(wcex),
			.lpfnWndProc = _WndProc,
			.hInstance = hInstance,
			.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP)),
			.hCursor = LoadCursor(nullptr, IDC_ARROW),
			.lpszClassName = CommonSharedConstants::MAIN_WINDOW_CLASS_NAME
		};
		RegisterClassEx(&wcex);

		wcex.style = CS_DBLCLKS;
		wcex.lpfnWndProc = _TitleBarWndProc;
		wcex.hIcon = NULL;
		wcex.lpszClassName = CommonSharedConstants::TITLE_BAR_WINDOW_CLASS_NAME;
		RegisterClassEx(&wcex);

		// 用于解决深色背景被视为透明的问题
		if (Win32Helper::GetOSVersion().IsWin10()) {
			BufferedPaintInit();
		}

		return Ignore();
	}();

	POINT windowPos;
	SIZE windowSize;
	MONITORINFO monitorInfo;
	_CreateWindow(windowPos, windowSize, monitorInfo);

	if (!Handle()) {
		return false;
	}

	_UpdateTheme();

	_rootPage = winrt::make_self<winrt::Magpie::implementation::RootPage>();

	// 初始化 XAML Islands
	_xamlSource = winrt::DesktopWindowXamlSource();
	_xamlSourceNative2 = _xamlSource.try_as<IDesktopWindowXamlSourceNative2>();
	_xamlSourceNative2->AttachToWindow(this->Handle());
	_xamlSourceNative2->get_WindowHandle(&_hwndXamlIsland);
	_xamlSource.Content(*_rootPage);

	// 焦点始终位于 _hwndXamlIsland 中
	_xamlSource.TakeFocusRequested(
		[](winrt::DesktopWindowXamlSource const& sender,
		winrt::DesktopWindowXamlSourceTakeFocusRequestedEventArgs const& args
	) {
		sender.NavigateFocus(args.Request());
	});

	// 使 XAML Islands 背景透明，从而显露出 DWM 绘制的背景。Win11 22H2 前我们使用纯色背景，
	// 这一步不是必需的，但也没坏处。
	XamlHelper::SetWindowBackgroundTransparency(winrt::Window::Current(), true);

	_isSmoothResizeEnabled = SmoothResizeHelper::EnableResizeSync(Handle(), App::Get());

	// 以最大化启动时 ShowWindow(Handle(), SW_SHOWMAXIMIZED) 会显示错误的动画，因此这里以
	// 以窗口化启动，但位置和尺寸都和最大化相同，显示完毕后将状态设为最大化。
	// FIXME: Win11 中以最大化启动时还是有动画错误，暂未找到解决方案。
	// 除此之外，SetWindowPos 还有其它作用：
	// 1. 延后设置窗口位置，同时设置初始 XAML Islands 窗口的尺寸
	// 2. 刷新窗口边框
	// 3. 防止窗口显示时背景闪烁: https://stackoverflow.com/questions/69715610/how-to-initialize-the-background-color-of-win32-app-to-something-other-than-whit
	if (AppSettings::Get().IsMainWindowMaximized()) {
		// 让 XAML Islands 窗口尺寸和最大化后相同
		RECT frame = monitorInfo.rcWork;
		AdjustWindowRectExForDpi(&frame, GetWindowStyle(Handle()), FALSE, 0, GetDpi());
		frame.top = monitorInfo.rcWork.top;
		// 窗口化时存在上边框，最大化时没有。更改 bottom 而不是 top 可以避免动画错误
		frame.bottom += _GetTopBorderThickness();

		SetWindowPos(
			Handle(),
			NULL,
			frame.left,
			frame.top,
			frame.right - frame.left,
			frame.bottom - frame.top,
			SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOCOPYBITS
		);

		// rcNormalPosition 使用工作区坐标
		POINT workspacePos = { windowPos.x + monitorInfo.rcWork.left , windowPos.y + monitorInfo.rcWork.top };

		// RootPage 加载完成后显示主窗口
		_rootPage->Loaded([this, workspacePos, windowSize](winrt::IInspectable const&, winrt::RoutedEventArgs const&) {
			// 显示窗口弹出动画
			ShowWindow(Handle(), SW_SHOWNORMAL);

			WINDOWPLACEMENT wp = { .length = sizeof(wp) };
			GetWindowPlacement(Handle(), &wp);

			// 必须调用两次 SetWindowPlacement，只调用一次会有中间状态，先变为窗口化尺寸然后最大化
			wp.showCmd = SW_SHOWMAXIMIZED;
			SetWindowPlacement(Handle(), &wp);

			wp.rcNormalPosition.left = workspacePos.x;
			wp.rcNormalPosition.top = workspacePos.y;
			wp.rcNormalPosition.right = workspacePos.x + windowSize.cx;
			wp.rcNormalPosition.bottom = workspacePos.y + windowSize.cy;
			SetWindowPlacement(Handle(), &wp);
		});
	} else {
		SetWindowPos(Handle(), NULL, windowPos.x, windowPos.y, windowSize.cx, windowSize.cy,
			SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOCOPYBITS);

		_rootPage->Loaded([this](winrt::IInspectable const&, winrt::RoutedEventArgs const&) {
			ShowWindow(Handle(), SW_SHOWNORMAL);
		});
	}

	// 创建标题栏窗口，它是主窗口的子窗口。我们将它置于 XAML Islands 窗口之上以防止鼠标事件被吞掉。
	// 
	// 出于未知的原因，必须添加 WS_EX_LAYERED 样式才能发挥作用，见
	// https://github.com/microsoft/terminal/blob/0ee2c74cd432eda153f3f3e77588164cde95044f/src/cascadia/WindowsTerminal/NonClientIslandWindow.cpp#L79
	// WS_EX_NOREDIRECTIONBITMAP 可以避免 WS_EX_LAYERED 导致的额外内存开销。
	//
	// WS_MINIMIZEBOX 和 WS_MAXIMIZEBOX 使得鼠标悬停时显示文字提示，Win11 的贴靠布局不依赖它们。
	CreateWindowEx(
		WS_EX_LAYERED | WS_EX_NOPARENTNOTIFY | WS_EX_NOREDIRECTIONBITMAP | WS_EX_NOACTIVATE,
		CommonSharedConstants::TITLE_BAR_WINDOW_CLASS_NAME,
		nullptr,
		WS_CHILD | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
		0, 0, 0, 0,
		Handle(),
		nullptr,
		wil::GetModuleInstanceHandle(),
		this
	);
	SetLayeredWindowAttributes(_hwndTitleBar.get(), 0, 255, LWA_ALPHA);

	if (Win32Helper::GetOSVersion().IsWin11()) {
		// 如果鼠标正位于一个按钮上，贴靠布局弹窗会出现在按钮下方。我们利用这个特性来修正贴靠布局弹窗的位置。
		// Win11 23H2 的某一次更新后，Snap Layout 不再依赖 UI Automation，而是依靠 WM_GETTITLEBARINFOEX
		// 消息来定位最大化按钮矩形。此行为破坏了许多程序的 Snap Layout 支持，好在 Win11 24H2 中问题得到了
		// 缓解。我们同时支持两种方案，以便在不同版本的 Win11 上都能正常工作。
		_hwndMaximizeButton = CreateWindowEx(
			WS_EX_NOPARENTNOTIFY,
			L"BUTTON",
			nullptr,
			WS_VISIBLE | WS_CHILD | WS_DISABLED | BS_OWNERDRAW,
			0, 0, 0, 0,
			_hwndTitleBar.get(),
			NULL,
			wil::GetModuleInstanceHandle(),
			NULL
		);

		// 允许 WM_GETTITLEBARINFOEX 通过 UIPI 防止以管理员身份运行时无法收到
		ChangeWindowMessageFilterEx(Handle(), WM_GETTITLEBARINFOEX, MSGFLT_ALLOW, nullptr);
	}

	_rootPage->TitleBar().LeftBottomPointChanged([this] {
		_ResizeTitleBarWindow();
	});

	_appThemeChangedRevoker = App::Get().ThemeChanged(
		winrt::auto_revoke, [this](bool) { _UpdateTheme(); });

	return true;
}

void MainWindow::Show() const noexcept {
	if (IsIconic(Handle())) {
		ShowWindow(Handle(), SW_RESTORE);
	}

	SetForegroundWindow(Handle());
}

void MainWindow::HandleMessage(const MSG& msg) {
	// XAML Islands 会吞掉 Alt+F4，需要特殊处理。见
	// https://github.com/microsoft/microsoft-ui-xaml/issues/2408
	if (msg.message == WM_SYSKEYDOWN && msg.wParam == VK_F4) [[unlikely]] {
		SendMessage(GetAncestor(msg.hwnd, GA_ROOT), msg.message, msg.wParam, msg.lParam);
		return;
	}

	if (_xamlSourceNative2) {
		BOOL processed = FALSE;
		HRESULT hr = _xamlSourceNative2->PreTranslateMessage(&msg, &processed);
		if (SUCCEEDED(hr) && processed) {
			return;
		}
	}

	TranslateMessage(&msg);
	DispatchMessage(&msg);
}

LRESULT MainWindow::_MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
	switch (msg) {
	case WM_SIZE:
	{
		base_type::_MessageHandler(WM_SIZE, wParam, lParam);

		if (wParam != SIZE_MINIMIZED && _rootPage) {
			if (_isSmoothResizeEnabled) {
				SmoothResizeHelper::SyncWindowSize(Handle(), App::Get());
			}

			// 调整 XAML Islands 窗口尺寸
			{
				int clientWidth = LOWORD(lParam);
				int clientHeight = HIWORD(lParam);
				// XAML Islands 窗口在上边框下方。Win10 和 Win11 中上边框都在客户区内。
				int topBorderThickness = (int)_GetTopBorderThickness();

				// SWP_NOZORDER 确保 XAML Islands 窗口始终在标题栏窗口下方，否则主窗口在调整大小时会闪烁
				SetWindowPos(
					_hwndXamlIsland,
					NULL,
					0,
					topBorderThickness,
					clientWidth,
					clientHeight - topBorderThickness,
					SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW
				);
			}

			_ResizeTitleBarWindow();

			_rootPage->TitleBar().CaptionButtons().IsWindowMaximized(_IsMaximized());

			// 使 ContentDialog 跟随窗口调整尺寸。来自
			// https://github.com/microsoft/microsoft-ui-xaml/issues/3577#issuecomment-1399250405
			if (winrt::CoreWindow coreWindow = winrt::CoreWindow::GetForCurrentThread()) {
				HWND hwndDWXS;
				coreWindow.try_as<ICoreWindowInterop>()->get_WindowHandle(&hwndDWXS);
				PostMessage(hwndDWXS, WM_SIZE, wParam, lParam);
			}

			App::Get().Dispatcher().TryEnqueue([xamlRoot(_rootPage->XamlRoot())]() {
				XamlHelper::RepositionXamlPopups(xamlRoot, true);
			});
		}

		return 0;
	}
	case WM_MOVING:
	{
		if (_rootPage) {
			XamlHelper::RepositionXamlPopups(_rootPage->XamlRoot(), false);
		}

		return 0;
	}
	case WM_GETMINMAXINFO:
	{
		// 设置窗口最小尺寸
		MINMAXINFO* mmi = (MINMAXINFO*)lParam;
		mmi->ptMinTrackSize = { 
			std::lround(500 * GetDpi() / float(USER_DEFAULT_SCREEN_DPI)),
			std::lround(300 * GetDpi() / float(USER_DEFAULT_SCREEN_DPI))
		};
		return 0;
	}
	case WM_NCRBUTTONUP:
	{
		// 我们自己处理标题栏右键，不知为何 DefWindowProc 没有作用
		if (wParam == HTCAPTION) {
			const POINT cursorPt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

			// 在标题栏上按下右键，在其他地方释放也会收到此消息。确保只有在标题栏上释放时才显示菜单
			RECT titleBarRect;
			GetWindowRect(_hwndTitleBar.get(), &titleBarRect);
			if (!Win32Helper::PtInRect(titleBarRect, cursorPt)) {
				break;
			}

			HMENU systemMenu = GetSystemMenu(Handle(), FALSE);

			// 根据窗口状态更新选项
			MENUITEMINFO mii{};
			mii.cbSize = sizeof(MENUITEMINFO);
			mii.fMask = MIIM_STATE;
			mii.fType = MFT_STRING;
			auto setState = [&](UINT item, bool enabled) {
				mii.fState = enabled ? MF_ENABLED : MF_DISABLED;
				SetMenuItemInfo(systemMenu, item, FALSE, &mii);
			};
			const bool isMaximized = _IsMaximized();
			setState(SC_RESTORE, isMaximized);
			setState(SC_MOVE, !isMaximized);
			setState(SC_SIZE, !isMaximized);
			setState(SC_MINIMIZE, true);
			setState(SC_MAXIMIZE, !isMaximized);
			setState(SC_CLOSE, true);
			SetMenuDefaultItem(systemMenu, UINT_MAX, FALSE);

			BOOL cmd = TrackPopupMenu(systemMenu, TPM_RETURNCMD, cursorPt.x, cursorPt.y, 0, Handle(), nullptr);
			if (cmd != 0) {
				PostMessage(Handle(), WM_SYSCOMMAND, cmd, 0);
			}
		}

		break;
	}
	case WM_ACTIVATE:
	{
		if (_rootPage) {
			_rootPage->TitleBar().IsWindowActive(LOWORD(wParam) != WA_INACTIVE);

			if (LOWORD(wParam) == WA_INACTIVE) {
				XamlHelper::CloseComboBoxPopup(_rootPage->XamlRoot());
			}
		}

		break;
	}
	case WM_GETTITLEBARINFOEX:
	{
		if (Win32Helper::GetOSVersion().IsWin11()) {
			// 为了支持 Win11 的贴靠布局，我们需要返回最大化按钮的矩形
			TITLEBARINFOEX* info = (TITLEBARINFOEX*)lParam;
			if (info->cbSize >= sizeof(TITLEBARINFOEX)) {
				base_type::_MessageHandler(msg, wParam, lParam);
				GetWindowRect(_hwndMaximizeButton, &info->rgrect[3]);
				return TRUE;
			}
		}

		break;
	}
	case WM_NCHITTEST:
	{
		// 为了和第三方程序兼容，确保主窗口本身可以正确响应 WM_NCHITTEST。
		// 见 https://github.com/microsoft/terminal/issues/8795

		// 基类处理非客户区
		LRESULT ht = base_type::_MessageHandler(msg, wParam, lParam);
		if (ht != HTCLIENT || !_hwndTitleBar) {
			return ht;
		}

		const POINT cursorPos = { GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam) };

		RECT clientRect;
		Win32Helper::GetClientScreenRect(Handle(), clientRect);

		// _hwndTitleBar 为标题栏区域，下方是客户区
		RECT titlebarWndRect{};
		GetWindowRect(_hwndTitleBar.get(), &titlebarWndRect);
		if (!PtInRect(&titlebarWndRect, cursorPos)) {
			return HTCLIENT;
		}

		static const winrt::Size buttonSizeInDips = [this]() {
			return _rootPage->TitleBar().CaptionButtons().CaptionButtonSize();
		}();

		float buttonWidthInPixels = buttonSizeInDips.Width * GetDpi() / USER_DEFAULT_SCREEN_DPI;
		float buttonHeightInPixels = buttonSizeInDips.Height * GetDpi() / USER_DEFAULT_SCREEN_DPI;

		if (cursorPos.y >= clientRect.top + _GetTopBorderThickness() + buttonHeightInPixels) {
			// 光标位于标题按钮下方，如果标题栏很宽，这里也可以拖动
			return HTCAPTION;
		}

		// 从右向左检查光标是否位于某个标题栏按钮上
		LONG cursorToRight = clientRect.right - cursorPos.x;
		if (cursorToRight < buttonWidthInPixels) {
			return HTCLOSE;
		} else if (cursorToRight < buttonWidthInPixels * 2) {
			// 支持 Win11 的贴靠布局
			return HTMAXBUTTON;
		} else if (cursorToRight < buttonWidthInPixels * 3) {
			return HTMINBUTTON;
		} else {
			// 不在任何标题栏按钮上则在可拖拽区域
			return HTCAPTION;
		}
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

		break;
	}
	case WM_SYSCOMMAND:
	{
		// 根据文档，wParam 的低四位供系统内部使用
		switch (wParam & 0xFFF0) {
		case SC_MINIMIZE:
		{
			// 最小化前关闭 ComboBox。不能在 WM_SIZE 中处理，该消息发送于最小化之后，会导致 ComboBox 无法交互
			if (_rootPage) {
				XamlHelper::CloseComboBoxPopup(_rootPage->XamlRoot());
			}
			break;
		}
		case SC_KEYMENU:
		{
			// 禁用按 Alt 键会激活窗口菜单的行为，它使用户界面无法交互
			if (lParam == 0) {
				return 0;
			}
			break;
		}
		}

		break;
	}
	case WM_DPICHANGED:
	{
		base_type::_MessageHandler(msg, wParam, lParam);
		DpiChanged.Invoke(GetDpi());
		return 0;
	}
	case WM_DESTROY:
	{
		AppSettings::Get().SaveAsync();

		_appThemeChangedRevoker.Revoke();
		// 标题栏窗口经常使用 Content()，确保在关闭 DWXS 前销毁
		_hwndTitleBar.reset();

		// 确保关闭过程中 _rootPage 已经为空
		_rootPage = nullptr;

		_xamlSourceNative2 = nullptr;
		// 必须手动重置 Content，否则会内存泄露，使 RootPage 无法析构
		_xamlSource.Content(nullptr);
		_xamlSource.Close();
		_xamlSource = nullptr;

		// 关闭 DesktopWindowXamlSource 后应清空消息队列以确保 RootPage 析构
		MSG msg1;
		while (PeekMessage(&msg1, nullptr, 0, 0, PM_REMOVE)) {
			DispatchMessage(&msg1);
		}
		// 偶尔清空消息队列无用，需要再清空一次，不确定是否 100% 可靠。谢谢你，XAML Islands！
		Sleep(0);
		while (PeekMessage(&msg1, nullptr, 0, 0, PM_REMOVE)) {
			DispatchMessage(&msg1);
		}

		base_type::_MessageHandler(msg, wParam, lParam);

		Destroyed.Invoke();

		// 不显示托盘图标时关闭主窗口应退出
		if (!AppSettings::Get().IsShowNotifyIcon()) {
			PostQuitMessage(0);
		}

		return 0;
	}
	}

	return base_type::_MessageHandler(msg, wParam, lParam);
}

bool MainWindow::_ShouldDrawBackground() const noexcept {
	return !Win32Helper::GetOSVersion().Is22H2OrNewer();
}

void MainWindow::_DrawBackground(HDC hdc, const RECT& bkgRect) const noexcept {
	assert(_ShouldDrawBackground() && _hbrBackground);

	// 绘制深色背景时需要注意使用 DwmExtendFrameIntoClientArea 后深色背景会被视为透明。解决方案来自
	// https://github.com/microsoft/terminal/blob/0ee2c74cd432eda153f3f3e77588164cde95044f/src/cascadia/WindowsTerminal/NonClientIslandWindow.cpp#L1030-L1047
	if (!App::Get().IsLightTheme() && Win32Helper::GetOSVersion().IsWin10()) {
		HDC opaqueDc;
		BP_PAINTPARAMS params = {
			.cbSize = sizeof(params),
			.dwFlags = BPPF_NOCLIP | BPPF_ERASE
		};
		HPAINTBUFFER buf = BeginBufferedPaint(hdc, &bkgRect, BPBF_TOPDOWNDIB, &params, &opaqueDc);
		if (buf && opaqueDc) {
			FillRect(opaqueDc, &bkgRect, _hbrBackground.get());
			BufferedPaintSetAlpha(buf, nullptr, 255);
			EndBufferedPaint(buf, TRUE);
		}
	} else {
		FillRect(hdc, &bkgRect, _hbrBackground.get());
	}
}

void MainWindow::_CreateWindow(POINT& windowPos, SIZE& windowSize, MONITORINFO& monitorInfo) noexcept {
	const winrt::Point& windowCenter = AppSettings::Get().MainWindowCenter();
	winrt::Size windowSizeInDips = AppSettings::Get().MainWindowSizeInDips();
	
	windowPos = { CW_USEDEFAULT,CW_USEDEFAULT };
	windowSize = {};

	// windowSizeInDips 小于零表示默认位置和尺寸
	if (windowSizeInDips.Width > 0) {
		// 检查窗口中心点的 DPI，根据我的测试，创建窗口时 Windows 使用窗口中心点确定 DPI。
		// 如果窗口中心点不在任何屏幕上，则查找最近的屏幕。如果窗口尺寸太大无法被屏幕容纳，
		// 则还原为默认位置和尺寸。
		HMONITOR hMon = MonitorFromPoint(
			{ std::lround(windowCenter.X),std::lround(windowCenter.Y) },
			MONITOR_DEFAULTTONEAREST
		);

		UINT dpi = USER_DEFAULT_SCREEN_DPI;
		GetDpiForMonitor(hMon, MDT_EFFECTIVE_DPI, &dpi, &dpi);

		const float dpiFactor = dpi / float(USER_DEFAULT_SCREEN_DPI);
		const winrt::Size windowSizeInPixels = {
			windowSizeInDips.Width * dpiFactor,
			windowSizeInDips.Height * dpiFactor
		};

		windowSize.cx = std::lround(windowSizeInPixels.Width);
		windowSize.cy = std::lround(windowSizeInPixels.Height);

		monitorInfo = { .cbSize = sizeof(MONITORINFO) };
		GetMonitorInfo(hMon, &monitorInfo);

		// 确保启动位置在屏幕工作区内。不允许启动时跨越多个屏幕
		if (windowSize.cx <= monitorInfo.rcWork.right - monitorInfo.rcWork.left &&
			windowSize.cy <= monitorInfo.rcWork.bottom - monitorInfo.rcWork.top) {
			windowPos.x = std::lround(windowCenter.X - windowSizeInPixels.Width / 2);
			windowPos.x = std::clamp(windowPos.x, monitorInfo.rcWork.left, monitorInfo.rcWork.right - windowSize.cx);

			windowPos.y = std::lround(windowCenter.Y - windowSizeInPixels.Height / 2);
			windowPos.y = std::clamp(windowPos.y, monitorInfo.rcWork.top, monitorInfo.rcWork.bottom - windowSize.cy);
		} else {
			// 屏幕工作区无法容纳窗口则使用默认窗口尺寸
			windowSize = {};
			windowSizeInDips.Width = -1.0f;
		}
	}

	CreateWindowEx(
		// Win11 22H2 中为了使用 Mica 背景需指定 WS_EX_NOREDIRECTIONBITMAP
		_ShouldDrawBackground() ? 0 : WS_EX_NOREDIRECTIONBITMAP,
		CommonSharedConstants::MAIN_WINDOW_CLASS_NAME,
		L"Magpie",
		WS_OVERLAPPEDWINDOW,
		windowPos.x,
		windowPos.y,
		// 尺寸为 0，有两个原因：一是可能无法确定尺寸，二是以最大化启动时需保持 XAML Islands 窗口尺寸稳定
		0,
		0,
		NULL,
		NULL,
		wil::GetModuleInstanceHandle(),
		this
	);
	assert(Handle());

	if (windowSize.cx != 0) {
		return;
	}

	HMONITOR hMon = MonitorFromWindow(Handle(), MONITOR_DEFAULTTONEAREST);

	monitorInfo = { .cbSize = sizeof(MONITORINFO) };
	GetMonitorInfo(hMon, &monitorInfo);

	const float dpiFactor = GetDpi() / float(USER_DEFAULT_SCREEN_DPI);
	const winrt::Size workingAreaSizeInDips = {
		(monitorInfo.rcWork.right - monitorInfo.rcWork.left) / dpiFactor,
		(monitorInfo.rcWork.bottom - monitorInfo.rcWork.top) / dpiFactor
	};

	// 确保启动尺寸小于屏幕工作区
	if (windowSizeInDips.Width <= 0 ||
		windowSizeInDips.Width > workingAreaSizeInDips.Width ||
		windowSizeInDips.Height > workingAreaSizeInDips.Height) {
		// 默认尺寸
		constexpr winrt::Size DEFAULT_SIZE = { 980.0f, 690.0f };

		windowSizeInDips = DEFAULT_SIZE;

		if (windowSizeInDips.Width > workingAreaSizeInDips.Width ||
			windowSizeInDips.Height > workingAreaSizeInDips.Height) {
			// 屏幕太小无法容纳默认尺寸
			windowSizeInDips.Width = workingAreaSizeInDips.Width * 0.8f;
			windowSizeInDips.Height = windowSizeInDips.Width * DEFAULT_SIZE.Height / DEFAULT_SIZE.Width;

			if (windowSizeInDips.Height > workingAreaSizeInDips.Height) {
				windowSizeInDips.Height = workingAreaSizeInDips.Height * 0.8f;
				windowSizeInDips.Width = windowSizeInDips.Height * DEFAULT_SIZE.Width / DEFAULT_SIZE.Height;
			}
		}
	}

	windowSize.cx = std::lround(windowSizeInDips.Width * dpiFactor);
	windowSize.cy = std::lround(windowSizeInDips.Height * dpiFactor);

	// 确保启动位置在屏幕工作区内
	RECT targetRect;
	GetWindowRect(Handle(), &targetRect);
	windowPos.x = std::clamp(targetRect.left, monitorInfo.rcWork.left, monitorInfo.rcWork.right - windowSize.cx);
	windowPos.y = std::clamp(targetRect.top, monitorInfo.rcWork.top, monitorInfo.rcWork.bottom - windowSize.cy);
}

void MainWindow::_UpdateTheme() noexcept {
	assert(Handle());

	const bool isLightTheme = App::Get().IsLightTheme();

	ThemeHelper::SetWindowTheme(
		Handle(),
		// Win10 中始终使用暗色边框，这也是 UWP 窗口的行为
		!isLightTheme || Win32Helper::GetOSVersion().IsWin10(),
		!isLightTheme
	);

	if (_ShouldDrawBackground()) {
		_hbrBackground.reset(CreateSolidBrush(isLightTheme ?
			ThemeHelper::LIGHT_TINT_COLOR : ThemeHelper::DARK_TINT_COLOR));
	} else {
		// 设置 Mica 背景
		DWM_SYSTEMBACKDROP_TYPE value = DWMSBT_MAINWINDOW;
		DwmSetWindowAttribute(Handle(), DWMWA_SYSTEMBACKDROP_TYPE, &value, sizeof(value));
	}
}

LRESULT MainWindow::_TitleBarWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
	if (msg == WM_NCCREATE) {
		MainWindow* that = (MainWindow*)(((CREATESTRUCT*)lParam)->lpCreateParams);
		assert(that && !that->_hwndTitleBar);
		that->_hwndTitleBar.reset(hWnd);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)that);
	} else if (MainWindow* that = (MainWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA)) {
		return that->_TitleBarMessageHandler(msg, wParam, lParam);
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT MainWindow::_TitleBarMessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
	switch (msg) {
	case WM_NCHITTEST:
	{
		// 和主窗口一致
		return _MessageHandler(WM_NCHITTEST, wParam, lParam);
	}
	// 在捕获光标时会收到
	case WM_MOUSEMOVE:
	{
		POINT cursorPos{ GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam) };
		ClientToScreen(_hwndTitleBar.get(), &cursorPos);
		wParam = _TitleBarMessageHandler(WM_NCHITTEST, 0, MAKELPARAM(cursorPos.x, cursorPos.y));
		[[fallthrough]];
	}
	case WM_NCMOUSEMOVE:
	{
		CaptionButtonsControl& captionButtons = _rootPage->TitleBar().CaptionButtons();

		// 将 hover 状态通知 CaptionButtons。标题栏窗口拦截了 XAML Islands 中的标题栏
		// 控件的鼠标消息，标题栏按钮的状态由我们手动控制。
		switch (wParam) {
		case HTTOP:
		case HTTOPLEFT:
		case HTTOPRIGHT:
		case HTCAPTION:
		{
			captionButtons.LeaveButtons();

			// 将这些消息传给主窗口才能移动窗口或者调整窗口大小
			return _MessageHandler(msg, wParam, lParam);
		}
		case HTMINBUTTON:
		case HTMAXBUTTON:
		case HTCLOSE:
			captionButtons.HoverButton((CaptionButton)wParam);

			// 追踪鼠标以确保鼠标离开标题栏时我们能收到 WM_NCMOUSELEAVE 消息，否则无法
			// 可靠的收到这个消息，尤其是在用户快速移动鼠标的时候。
			if (!_isTrackingMouse && msg == WM_NCMOUSEMOVE) {
				TRACKMOUSEEVENT ev{};
				ev.cbSize = sizeof(TRACKMOUSEEVENT);
				ev.dwFlags = TME_LEAVE | TME_NONCLIENT;
				ev.hwndTrack = _hwndTitleBar.get();
				ev.dwHoverTime = HOVER_DEFAULT; // 不关心 HOVER 消息
				TrackMouseEvent(&ev);
				_isTrackingMouse = true;
			}

			break;
		default:
			captionButtons.LeaveButtons();
		}
		break;
	}
	case WM_NCMOUSELEAVE:
	case WM_MOUSELEAVE:
	{
		// 我们需要检查鼠标是否**真的**离开了标题栏按钮，因为在某些情况下 OS 会错误汇报。
		// 比如: 鼠标在关闭按钮上停留了一段时间，系统会显示文字提示，这时按下左键，便会收
		// 到 WM_NCMOUSELEAVE，但此时鼠标并没有离开标题栏按钮
		POINT cursorPos;
		GetCursorPos(&cursorPos);
		// 先检查鼠标是否在主窗口上，如果正在显示文字提示，会返回 _hwndTitleBar
		HWND hwndUnderCursor = WindowFromPoint(cursorPos);
		if (hwndUnderCursor != Handle() && hwndUnderCursor != _hwndTitleBar.get()) {
			_rootPage->TitleBar().CaptionButtons().LeaveButtons();
		} else {
			// 然后检查鼠标在标题栏上的位置
			LRESULT hit = _TitleBarMessageHandler(WM_NCHITTEST, 0, MAKELPARAM(cursorPos.x, cursorPos.y));
			if (hit != HTMINBUTTON && hit != HTMAXBUTTON && hit != HTCLOSE) {
				_rootPage->TitleBar().CaptionButtons().LeaveButtons();
			}
		}

		_isTrackingMouse = false;
		break;
	}
	case WM_NCLBUTTONDOWN:
	case WM_NCLBUTTONDBLCLK:
	{
		// 手动处理标题栏上的点击。如果在标题栏按钮上，则通知 CaptionButtons，否则将消息传递给主窗口
		switch (wParam) {
		case HTTOP:
		case HTTOPLEFT:
		case HTTOPRIGHT:
		case HTCAPTION:
		{
			// 将这些消息传给主窗口才能移动窗口或者调整窗口大小
			return _MessageHandler(msg, wParam, lParam);
		}
		case HTMINBUTTON:
		case HTMAXBUTTON:
		case HTCLOSE:
			_rootPage->TitleBar().CaptionButtons().PressButton((CaptionButton)wParam);
			// 在标题栏按钮上按下左键后我们便捕获光标，这样才能在释放时得到通知。注意捕获光标后
			// 便不会再收到 NC 族消息，这就是为什么我们要处理 WM_MOUSEMOVE 和 WM_LBUTTONUP
			SetCapture(_hwndTitleBar.get());
			break;
		}
		return 0;
	}
	// 在捕获光标时会收到
	case WM_LBUTTONUP:
	{
		ReleaseCapture();

		POINT cursorPos{ GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam) };
		ClientToScreen(_hwndTitleBar.get(), &cursorPos);
		wParam = _TitleBarMessageHandler(WM_NCHITTEST, 0, MAKELPARAM(cursorPos.x, cursorPos.y));
		[[fallthrough]];
	}
	case WM_NCLBUTTONUP:
	{
		// 处理鼠标在标题栏上释放。如果在标题栏按钮上，则通知 CaptionButtons，否则将消息传递给主窗口
		switch (wParam) {
		case HTTOP:
		case HTTOPLEFT:
		case HTTOPRIGHT:
		case HTCAPTION:
		{
			// 在可拖拽区域或上边框释放左键，将此消息传递给主窗口
			_rootPage->TitleBar().CaptionButtons().ReleaseButtons();
			return _MessageHandler(msg, wParam, lParam);
		}
		case HTMINBUTTON:
		case HTMAXBUTTON:
		case HTCLOSE:
			// 在标题栏按钮上释放左键
			_rootPage->TitleBar().CaptionButtons().ReleaseButton((CaptionButton)wParam);
			break;
		default:
			_rootPage->TitleBar().CaptionButtons().ReleaseButtons();
		}
		
		return 0;
	}
	case WM_NCRBUTTONDOWN:
	case WM_NCRBUTTONDBLCLK:
	case WM_NCRBUTTONUP:
		// 不关心右键，将它们传递给主窗口
		return _MessageHandler(msg, wParam, lParam);
	}

	return DefWindowProc(_hwndTitleBar.get(), msg, wParam, lParam);
}

void MainWindow::_ResizeTitleBarWindow() noexcept {
	if (!_hwndTitleBar.get()) {
		return;
	}

	// 将标题栏窗口置于 XAML Islands 窗口上方，覆盖上边框和标题栏控件
	const winrt::Point leftBottom = _rootPage->TitleBar().LeftBottomPoint();
	const float dpiScale = GetDpi() / float(USER_DEFAULT_SCREEN_DPI);

	const int titleBarX = (int)std::floorf(leftBottom.X * dpiScale);

	// 右边界使用客户区边界
	RECT clientRect;
	GetClientRect(Handle(), &clientRect);
	const int titleBarWidth = clientRect.right - titleBarX;

	const uint32_t topBorderHeight = _GetTopBorderThickness();
	// 不知为何，直接向上取整有时无法遮盖 TitleBarControl
	const int titleBarHeight = topBorderHeight + (int)std::floorf(leftBottom.Y * dpiScale + 1);

	SetWindowPos(
		_hwndTitleBar.get(),
		HWND_TOP,
		titleBarX, 0, titleBarWidth, titleBarHeight,
		SWP_SHOWWINDOW
	);

	if (_hwndMaximizeButton) {
		static const float captionButtonHeightInDips = [&]() {
			return _rootPage->TitleBar().CaptionButtons().CaptionButtonSize().Height;
		}();

		const int captionButtonHeightInPixels = (int)std::ceilf(captionButtonHeightInDips * dpiScale);

		// 确保原生按钮和标题栏按钮高度相同
		MoveWindow(_hwndMaximizeButton, titleBarX, topBorderHeight, titleBarWidth, captionButtonHeightInPixels, FALSE);
	}

	// 设置标题栏窗口的最大化样式，这样才能展示正确的文字提示
	LONG_PTR style = GetWindowLongPtr(_hwndTitleBar.get(), GWL_STYLE);
	SetWindowLongPtr(_hwndTitleBar.get(), GWL_STYLE,
		_IsMaximized() ? style | WS_MAXIMIZE : style & ~WS_MAXIMIZE);
}

}
