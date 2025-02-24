#include "pch.h"
#include "MainWindow.h"
#include "CommonSharedConstants.h"
#include "Win32Helper.h"
#include "ThemeHelper.h"
#include <ShellScalingApi.h>
#include "resource.h"
#include "EffectsService.h"
#include "AppSettings.h"
#include "App.h"
#include "CaptionButtonsControl.h"
#include "TitleBarControl.h"

using namespace winrt;
using namespace winrt::Magpie::implementation;

namespace Magpie {

bool MainWindow::Create() noexcept {
	static Ignore _ = [] {
		const HINSTANCE hInstance = wil::GetModuleInstanceHandle();

		WNDCLASSEXW wcex{
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

		return Ignore();
	}();

	const auto& [posToSet, sizeToSet] = _CreateWindow();

	if (!Handle()) {
		return false;
	}

	_Content(make_self<RootPage>());

	_appThemeChangedRevoker = App::Get().ThemeChanged(auto_revoke, [this](bool) { _UpdateTheme(); });
	_UpdateTheme();
	
	_SetInitialMaximized(AppSettings::Get().IsMainWindowMaximized());

	// 1. 设置初始 XAML Islands 窗口的尺寸
	// 2. 刷新窗口边框
	// 3. 无法获知 DPI 的情况下 _CreateWindow 创建的窗口尺寸为零，在这里延后设置窗口位置
	// 4. 防止窗口显示时背景闪烁: https://stackoverflow.com/questions/69715610/how-to-initialize-the-background-color-of-win32-app-to-something-other-than-whit
	SetWindowPos(Handle(), NULL, posToSet.x, posToSet.y, sizeToSet.cx, sizeToSet.cy,
		(sizeToSet.cx == 0 ? (SWP_NOMOVE | SWP_NOSIZE) : 0) | SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOCOPYBITS);

	// Xaml 控件加载完成后显示主窗口
	Content()->Loaded([this](winrt::IInspectable const&, RoutedEventArgs const&) {
		if (AppSettings::Get().IsMainWindowMaximized()) {
			// ShowWindow(Handle(), SW_SHOWMAXIMIZED) 会显示错误的动画。因此我们以窗口化显示，
			// 但位置和大小都和最大化相同，显示完毕后将状态设为最大化。
			// 
			// 在此过程中，_isMaximized 始终是 true。

			// 保存原始窗口化位置
			WINDOWPLACEMENT wp{ .length = sizeof(wp) };
			GetWindowPlacement(Handle(), &wp);

			// 查询最大化窗口位置
			if (HMONITOR hMon = MonitorFromWindow(Handle(), MONITOR_DEFAULTTONEAREST)) {
				MONITORINFO mi{ .cbSize = sizeof(mi) };
				GetMonitorInfo(hMon, &mi);

				// 播放窗口显示动画
				SetWindowPos(
					Handle(),
					NULL,
					mi.rcWork.left,
					mi.rcWork.top,
					mi.rcMonitor.right - mi.rcMonitor.left,
					mi.rcMonitor.bottom - mi.rcMonitor.top,
					SWP_SHOWWINDOW
				);
			}

			// 将状态设为最大化，也还原了原始的窗口化位置
			wp.showCmd = SW_SHOWMAXIMIZED;
			SetWindowPlacement(Handle(), &wp);

			_SetInitialMaximized(false);
		} else {
			ShowWindow(Handle(), SW_SHOWNORMAL);
		}

		Win32Helper::SetForegroundWindow(Handle());
	});

	const HINSTANCE hInstance = wil::GetModuleInstanceHandle();

	// 创建标题栏窗口，它是主窗口的子窗口。我们将它置于 XAML Islands 窗口之上以防止鼠标事件被吞掉
	// 
	// 出于未知的原因，必须添加 WS_EX_LAYERED 样式才能发挥作用，见
	// https://github.com/microsoft/terminal/blob/0ee2c74cd432eda153f3f3e77588164cde95044f/src/cascadia/WindowsTerminal/NonClientIslandWindow.cpp#L79
	// WS_EX_NOREDIRECTIONBITMAP 可以避免 WS_EX_LAYERED 导致的额外内存开销
	//
	// WS_MINIMIZEBOX 和 WS_MAXIMIZEBOX 使得鼠标悬停时显示文字提示，Win11 的贴靠布局不依赖它们
	CreateWindowEx(
		WS_EX_LAYERED | WS_EX_NOPARENTNOTIFY | WS_EX_NOREDIRECTIONBITMAP | WS_EX_NOACTIVATE,
		CommonSharedConstants::TITLE_BAR_WINDOW_CLASS_NAME,
		L"",
		WS_CHILD | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
		0, 0, 0, 0,
		Handle(),
		nullptr,
		hInstance,
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
			L"",
			WS_VISIBLE | WS_CHILD | WS_DISABLED | BS_OWNERDRAW,
			0, 0, 0, 0,
			_hwndTitleBar.get(),
			NULL,
			hInstance,
			NULL
		);

		// 允许 WM_GETTITLEBARINFOEX 通过 UIPI 防止以管理员身份运行时无法收到
		ChangeWindowMessageFilterEx(Handle(), WM_GETTITLEBARINFOEX, MSGFLT_ALLOW, nullptr);
	}

	Content()->TitleBar().SizeChanged([this](winrt::IInspectable const&, SizeChangedEventArgs const&) {
		_ResizeTitleBarWindow();
	});

	return true;
}

void MainWindow::Show() const noexcept {
	if (IsIconic(Handle())) {
		ShowWindow(Handle(), SW_RESTORE);
	}

	Win32Helper::SetForegroundWindow(Handle());
}

LRESULT MainWindow::_MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
	switch (msg) {
	case WM_SIZE:
	{
		LRESULT ret = base_type::_MessageHandler(WM_SIZE, wParam, lParam);
		_ResizeTitleBarWindow();

		// 以最大化显示时实际上是先窗口化显示然后改为最大化，确保最大化按钮状态正确
		Content()->TitleBar().CaptionButtons().IsWindowMaximized(
			_IsMaximized() || _IsInitialMaximized());
		return ret;
	}
	case WM_GETMINMAXINFO:
	{
		// 设置窗口最小尺寸
		MINMAXINFO* mmi = (MINMAXINFO*)lParam;
		mmi->ptMinTrackSize = { 
			std::lroundf(500 * CurrentDpi() / float(USER_DEFAULT_SCREEN_DPI)),
			std::lroundf(300 * CurrentDpi() / float(USER_DEFAULT_SCREEN_DPI))
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
			if (!PtInRect(&titleBarRect, cursorPt)) {
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
		if (Content()) {
			Content()->TitleBar().IsWindowActive(LOWORD(wParam) != WA_INACTIVE);
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
		if (_hwndTitleBar) {
			// 自行处理标题栏区域，剩下的交给 OS
			LRESULT ht = _TitleBarMessageHandler(WM_NCHITTEST, 0, lParam);
			if (ht != HTNOWHERE) {
				return ht;
			}
		}
		break;
	}
	case WM_DESTROY:
	{
		AppSettings::Get().Save();
		_appThemeChangedRevoker.Revoke();
		// 标题栏窗口经常使用 Content()，确保在关闭 DWXS 前销毁
		_hwndTitleBar.reset();
		_trackingMouse = false;

		// 不显示托盘图标时关闭主窗口应退出
		if (!AppSettings::Get().IsShowNotifyIcon()) {
			LRESULT ret = base_type::_MessageHandler(msg, wParam, lParam);
			// 由于基类会清空消息队列，PostQuitMessage 应在基类处理完毕后执行
			PostQuitMessage(0);
			return ret;
		}

		break;
	}
	}
	return base_type::_MessageHandler(msg, wParam, lParam);
}

std::pair<POINT, SIZE> MainWindow::_CreateWindow() noexcept {
	const Point& windowCenter = AppSettings::Get().MainWindowCenter();
	Size windowSizeInDips = AppSettings::Get().MainWindowSizeInDips();

	POINT windowPos = { CW_USEDEFAULT,CW_USEDEFAULT };
	SIZE windowSize{};

	// windowSizeInDips 小于零表示默认位置和尺寸
	if (windowSizeInDips.Width > 0) {
		// 检查窗口中心点的 DPI，根据我的测试，创建窗口时 Windows 使用窗口中心点确定 DPI。
		// 如果窗口中心点不在任何屏幕上，则查找最近的屏幕。如果窗口尺寸太大无法被屏幕容纳，
		// 则还原为默认位置和尺寸。
		const HMONITOR hMon = MonitorFromPoint(
			{ std::lroundf(windowCenter.X),std::lroundf(windowCenter.Y) },
			MONITOR_DEFAULTTONEAREST
		);

		UINT dpi = USER_DEFAULT_SCREEN_DPI;
		GetDpiForMonitor(hMon, MDT_EFFECTIVE_DPI, &dpi, &dpi);

		const float dpiFactor = dpi / float(USER_DEFAULT_SCREEN_DPI);
		const Size windowSizeInPixels = {
			windowSizeInDips.Width * dpiFactor,
			windowSizeInDips.Height * dpiFactor
		};

		windowSize.cx = std::lroundf(windowSizeInPixels.Width);
		windowSize.cy = std::lroundf(windowSizeInPixels.Height);

		MONITORINFO mi{ .cbSize = sizeof(mi) };
		GetMonitorInfo(hMon, &mi);

		// 确保启动位置在屏幕工作区内。不允许启动时跨越多个屏幕
		if (windowSize.cx <= mi.rcWork.right - mi.rcWork.left && windowSize.cy <= mi.rcWork.bottom - mi.rcWork.top) {
			windowPos.x = std::lroundf(windowCenter.X - windowSizeInPixels.Width / 2);
			windowPos.x = std::clamp(windowPos.x, mi.rcWork.left, mi.rcWork.right - windowSize.cx);

			windowPos.y = std::lroundf(windowCenter.Y - windowSizeInPixels.Height / 2);
			windowPos.y = std::clamp(windowPos.y, mi.rcWork.top, mi.rcWork.bottom - windowSize.cy);
		} else {
			// 屏幕工作区无法容纳窗口则使用默认窗口尺寸
			windowSize = {};
			windowSizeInDips.Width = -1.0f;
		}
	}

	// Win11 22H2 中为了使用 Mica 背景需指定 WS_EX_NOREDIRECTIONBITMAP
	// windowSize 可能为零，并返回窗口尺寸给调用者
	CreateWindowEx(
		Win32Helper::GetOSVersion().Is22H2OrNewer() ? WS_EX_NOREDIRECTIONBITMAP : 0,
		CommonSharedConstants::MAIN_WINDOW_CLASS_NAME,
		L"Magpie",
		WS_OVERLAPPEDWINDOW,
		windowPos.x,
		windowPos.y,
		windowSize.cx,
		windowSize.cy,
		NULL,
		NULL,
		wil::GetModuleInstanceHandle(),
		this
	);
	assert(Handle());

	if (windowSize.cx == 0) {
		const HMONITOR hMon = MonitorFromWindow(Handle(), MONITOR_DEFAULTTONEAREST);

		MONITORINFO mi{ .cbSize = sizeof(mi) };
		GetMonitorInfo(hMon, &mi);

		const float dpiFactor = CurrentDpi() / float(USER_DEFAULT_SCREEN_DPI);
		const Size workingAreaSizeInDips = {
			(mi.rcWork.right - mi.rcWork.left) / dpiFactor,
			(mi.rcWork.bottom - mi.rcWork.top) / dpiFactor
		};

		// 确保启动尺寸小于屏幕工作区
		if (windowSizeInDips.Width <= 0 ||
			windowSizeInDips.Width > workingAreaSizeInDips.Width ||
			windowSizeInDips.Height > workingAreaSizeInDips.Height) {
			// 默认尺寸
			static constexpr Size DEFAULT_SIZE{ 980.0f, 690.0f };

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

		windowSize.cx = std::lroundf(windowSizeInDips.Width * dpiFactor);
		windowSize.cy = std::lroundf(windowSizeInDips.Height * dpiFactor);

		// 确保启动位置在屏幕工作区内
		RECT targetRect;
		GetWindowRect(Handle(), &targetRect);
		windowPos.x = std::clamp(targetRect.left, mi.rcWork.left, mi.rcWork.right - windowSize.cx);
		windowPos.y = std::clamp(targetRect.top, mi.rcWork.top, mi.rcWork.bottom - windowSize.cy);

		return std::make_pair(windowPos, windowSize);
	} else {
		return {};
	}
}

void MainWindow::_UpdateTheme() noexcept {
	XamlWindowT::_SetTheme(App::Get().IsLightTheme());
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
	case WM_CTLCOLORBTN:
	{
		// 使原生按钮控件透明，虽然整个标题栏窗口都是不可见的
		return NULL;
	}
	case WM_NCHITTEST:
	{
		POINT cursorPos{ GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam) };
		ScreenToClient(_hwndTitleBar.get(), &cursorPos);

		RECT titleBarClientRect;
		GetClientRect(_hwndTitleBar.get(), &titleBarClientRect);
		if (!PtInRect(&titleBarClientRect, cursorPos)) {
			// 先检查鼠标是否在窗口内。在标题栏按钮上按下鼠标时我们会捕获光标，从而收到 WM_MOUSEMOVE 和 WM_LBUTTONUP 消息。
			// 它们使用 WM_NCHITTEST 测试鼠标位于哪个区域
			return HTNOWHERE;
		}

		if (!_IsMaximized()) {
			const int resizeHandleHeight = _GetResizeHandleHeight();
			if (cursorPos.y < resizeHandleHeight) {
				// 鼠标位于上边框
				if (cursorPos.x < resizeHandleHeight) {
					return HTTOPLEFT;
				} else if (cursorPos.x + resizeHandleHeight >= titleBarClientRect.right) {
					return HTTOPRIGHT;
				} else {
					return HTTOP;
				}
			}
		}
		
		static const Size buttonSizeInDips = [this]() {
			return Content()->TitleBar().CaptionButtons().CaptionButtonSize();
		}();

		const float buttonWidthInPixels = buttonSizeInDips.Width * CurrentDpi() / USER_DEFAULT_SCREEN_DPI;
		const float buttonHeightInPixels = buttonSizeInDips.Height * CurrentDpi() / USER_DEFAULT_SCREEN_DPI;

		if (cursorPos.y >= _GetTopBorderThickness() + buttonHeightInPixels) {
			// 鼠标位于标题按钮下方，如果标题栏很宽，这里也可以拖动
			return HTCAPTION;
		}

		// 从右向左检查鼠标是否位于某个标题栏按钮上
		const LONG cursorToRight = titleBarClientRect.right - cursorPos.x;
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
		CaptionButtonsControl& captionButtons = Content()->TitleBar().CaptionButtons();

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
			if (!_trackingMouse && msg == WM_NCMOUSEMOVE) {
				TRACKMOUSEEVENT ev{};
				ev.cbSize = sizeof(TRACKMOUSEEVENT);
				ev.dwFlags = TME_LEAVE | TME_NONCLIENT;
				ev.hwndTrack = _hwndTitleBar.get();
				ev.dwHoverTime = HOVER_DEFAULT; // 不关心 HOVER 消息
				TrackMouseEvent(&ev);
				_trackingMouse = true;
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
			Content()->TitleBar().CaptionButtons().LeaveButtons();
		} else {
			// 然后检查鼠标在标题栏上的位置
			LRESULT hit = _TitleBarMessageHandler(WM_NCHITTEST, 0, MAKELPARAM(cursorPos.x, cursorPos.y));
			if (hit != HTMINBUTTON && hit != HTMAXBUTTON && hit != HTCLOSE) {
				Content()->TitleBar().CaptionButtons().LeaveButtons();
			}
		}

		_trackingMouse = false;
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
			Content()->TitleBar().CaptionButtons().PressButton((CaptionButton)wParam);
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
			Content()->TitleBar().CaptionButtons().ReleaseButtons();
			return _MessageHandler(msg, wParam, lParam);
		}
		case HTMINBUTTON:
		case HTMAXBUTTON:
		case HTCLOSE:
			// 在标题栏按钮上释放左键
			Content()->TitleBar().CaptionButtons().ReleaseButton((CaptionButton)wParam);
			break;
		default:
			Content()->TitleBar().CaptionButtons().ReleaseButtons();
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

	TitleBarControl& titleBar = Content()->TitleBar();

	// 获取标题栏的边框矩形
	Rect rect{0.0f, 0.0f, (float)titleBar.ActualWidth(), (float)titleBar.ActualHeight()};
	rect = titleBar.TransformToVisual(*Content()).TransformBounds(rect);

	const float dpiScale = CurrentDpi() / float(USER_DEFAULT_SCREEN_DPI);
	const uint32_t topBorderHeight = _GetTopBorderThickness();

	// 将标题栏窗口置于 XAML Islands 窗口上方，覆盖上边框和标题栏控件
	const int titleBarX = (int)std::floorf(rect.X * dpiScale);
	const int titleBarWidth = (int)std::ceilf(rect.Width * dpiScale);
	SetWindowPos(
		_hwndTitleBar.get(),
		HWND_TOP,
		titleBarX,
		0,
		titleBarWidth,
		topBorderHeight + (int)std::floorf(rect.Height * dpiScale + 1),	// 不知为何，直接向上取整有时无法遮盖 TitleBarControl
		SWP_SHOWWINDOW
	);

	if (_hwndMaximizeButton) {
		static const float captionButtonHeightInDips = [&]() {
			return titleBar.CaptionButtons().CaptionButtonSize().Height;
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
