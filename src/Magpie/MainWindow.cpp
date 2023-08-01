#include "pch.h"
#include "MainWindow.h"
#include "CommonSharedConstants.h"
#include "Win32Utils.h"
#include "ThemeHelper.h"
#include "XamlApp.h"

namespace Magpie {

bool MainWindow::Create(HINSTANCE hInstance, const RECT& windowRect, bool isMaximized) noexcept {
	static const int _ = [](HINSTANCE hInstance) {
		WNDCLASSEXW wcex{};
		wcex.cbSize = sizeof(wcex);
		wcex.lpfnWndProc = _WndProc;
		wcex.hInstance = hInstance;
		wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(CommonSharedConstants::IDI_APP));
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.lpszClassName = CommonSharedConstants::MAIN_WINDOW_CLASS_NAME;
		RegisterClassEx(&wcex);

		wcex.style = CS_DBLCLKS;
		wcex.lpfnWndProc = _TitleBarWndProc;
		wcex.hIcon = NULL;
		wcex.lpszClassName = CommonSharedConstants::TITLE_BAR_WINDOW_CLASS_NAME;
		RegisterClassEx(&wcex);

		return 0;
	}(hInstance);

	// Win11 22H2 中为了使用 Mica 背景需指定 WS_EX_NOREDIRECTIONBITMAP
	CreateWindowEx(
		Win32Utils::GetOSVersion().Is22H2OrNewer() ? WS_EX_NOREDIRECTIONBITMAP : 0,
		CommonSharedConstants::MAIN_WINDOW_CLASS_NAME,
		L"Magpie",
		WS_OVERLAPPEDWINDOW,
		windowRect.left, windowRect.top, windowRect.right, windowRect.bottom,
		NULL,
		NULL,
		hInstance,
		this
	);

	if (!_hWnd) {
		return false;
	}

	_SetContent(winrt::Magpie::App::MainPage());

	_content.ActualThemeChanged([this](winrt::FrameworkElement const&, winrt::IInspectable const&) {
		_UpdateTheme();
	});
	_UpdateTheme();

	// 窗口尚未显示无法最大化，所以我们设置 _isMaximized 使 XamlWindow 估计 XAML Islands 窗口尺寸。
	// 否则在显示窗口时可能会看到 NavigationView 的导航栏的展开动画。
	_isMaximized = isMaximized;

	// 1. 设置初始 XAML Islands 窗口的尺寸
	// 2. 刷新窗口边框
	// 3. 防止窗口显示时背景闪烁: https://stackoverflow.com/questions/69715610/how-to-initialize-the-background-color-of-win32-app-to-something-other-than-whit
	SetWindowPos(_hWnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

	// Xaml 控件加载完成后显示主窗口
	_content.Loaded([this, isMaximized](winrt::IInspectable const&, winrt::RoutedEventArgs const&) {
		if (isMaximized) {
			// ShowWindow(_hWnd, SW_SHOWMAXIMIZED) 会显示错误的动画。因此我们以窗口化显示，
			// 但位置和大小都和最大化相同，显示完毕后将状态设为最大化。
			// 
			// 在此过程中，_isMaximized 始终是 true。

			// 保存原始窗口化位置
			WINDOWPLACEMENT wp{};
			wp.length = sizeof(wp);
			GetWindowPlacement(_hWnd, &wp);

			// 查询最大化窗口位置
			if (HMONITOR hMon = MonitorFromWindow(_hWnd, MONITOR_DEFAULTTONEAREST)) {
				MONITORINFO mi{};
				mi.cbSize = sizeof(mi);
				GetMonitorInfo(hMon, &mi);

				// 播放窗口显示动画
				SetWindowPos(
					_hWnd,
					NULL,
					mi.rcWork.left,
					mi.rcWork.top,
					mi.rcMonitor.right - mi.rcMonitor.left,
					mi.rcMonitor.bottom - mi.rcMonitor.top,
					SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW
				);
			}

			// 将状态设为最大化，也还原了原始的窗口化位置
			wp.showCmd = SW_SHOWMAXIMIZED;
			SetWindowPlacement(_hWnd, &wp);
		} else {
			ShowWindow(_hWnd, SW_SHOWNORMAL);
		}

		Win32Utils::SetForegroundWindow(_hWnd);

		_isWindowShown = true;
	});

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
		_hWnd,
		nullptr,
		hInstance,
		this
	);
	SetLayeredWindowAttributes(_hwndTitleBar, 0, 255, LWA_ALPHA);

	_content.TitleBar().SizeChanged([this](winrt::IInspectable const&, winrt::SizeChangedEventArgs const&) {
		_ResizeTitleBarWindow();
	});

	return true;
}

void MainWindow::Show() const noexcept {
	if (IsIconic(_hWnd)) {
		ShowWindow(_hWnd, SW_RESTORE);
	}

	Win32Utils::SetForegroundWindow(_hWnd);
}

LRESULT MainWindow::_MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
	switch (msg) {
	case WM_SIZE:
	{
		LRESULT ret = base_type::_MessageHandler(WM_SIZE, wParam, lParam);
		_ResizeTitleBarWindow();
		_content.TitleBar().CaptionButtons().IsWindowMaximized(_isMaximized);
		return ret;
	}
	case WM_GETMINMAXINFO:
	{
		// 设置窗口最小尺寸
		MINMAXINFO* mmi = (MINMAXINFO*)lParam;
		mmi->ptMinTrackSize = { 
			std::lround(550 * _currentDpi / double(USER_DEFAULT_SCREEN_DPI)),
			std::lround(300 * _currentDpi / double(USER_DEFAULT_SCREEN_DPI))
		};
		return 0;
	}
	case WM_NCRBUTTONUP:
	{
		// 我们自己处理标题栏右键，不知为何 DefWindowProc 没有作用
		if (wParam == HTCAPTION) {
			HMENU systemMenu = GetSystemMenu(_hWnd, FALSE);

			// 根据窗口状态更新选项
			MENUITEMINFO mii{};
			mii.cbSize = sizeof(MENUITEMINFO);
			mii.fMask = MIIM_STATE;
			mii.fType = MFT_STRING;
			auto setState = [&](UINT item, bool enabled) {
				mii.fState = enabled ? MF_ENABLED : MF_DISABLED;
				SetMenuItemInfo(systemMenu, item, FALSE, &mii);
			};
			setState(SC_RESTORE, _isMaximized);
			setState(SC_MOVE, !_isMaximized);
			setState(SC_SIZE, !_isMaximized);
			setState(SC_MINIMIZE, true);
			setState(SC_MAXIMIZE, !_isMaximized);
			setState(SC_CLOSE, true);
			SetMenuDefaultItem(systemMenu, UINT_MAX, FALSE);

			BOOL cmd = TrackPopupMenu(systemMenu, TPM_RETURNCMD,
				GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, _hWnd, nullptr);
			if (cmd != 0) {
				PostMessage(_hWnd, WM_SYSCOMMAND, cmd, 0);
			}
		}
		break;
	}
	case WM_ACTIVATE:
	{
		_content.TitleBar().IsWindowActive(LOWORD(wParam) != WA_INACTIVE);
		break;
	}
	case WM_DESTROY:
	{
		XamlApp::Get().SaveSettings();
		_hwndTitleBar = NULL;
		_trackingMouse = false;
		break;
	}
	case CommonSharedConstants::WM_QUIT_MAGPIE:
	{
		XamlApp::Get().Quit();
		return 0;
	}
	case CommonSharedConstants::WM_RESTART_MAGPIE:
	{
		XamlApp::Get().Restart(false);
		return 0;
	}
	}
	return base_type::_MessageHandler(msg, wParam, lParam);
}

void MainWindow::_UpdateTheme() {
	XamlWindowT::_SetTheme(_content.ActualTheme() == winrt::ElementTheme::Dark);
}

LRESULT MainWindow::_TitleBarWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
	if (msg == WM_NCCREATE) {
		MainWindow* that = (MainWindow*)(((CREATESTRUCT*)lParam)->lpCreateParams);
		assert(that && !that->_hwndTitleBar);
		that->_hwndTitleBar = hWnd;
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
		POINT cursorPos{ GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam) };
		ScreenToClient(_hwndTitleBar, &cursorPos);

		RECT titleBarClientRect;
		GetClientRect(_hwndTitleBar, &titleBarClientRect);
		if (!PtInRect(&titleBarClientRect, cursorPos)) {
			// 先检查鼠标是否在窗口内。在标题栏按钮上按下鼠标时我们会捕获光标，从而收到 WM_MOUSEMOVE 和 WM_LBUTTONUP 消息。
			// 它们使用 WM_NCHITTEST 测试鼠标位于哪个区域
			return HTNOWHERE;
		}

		if (!_isMaximized && cursorPos.y + (int)_GetTopBorderHeight() < _GetResizeHandleHeight()) {
			// 鼠标位于上边框
			return HTTOP;
		}
		
		static const winrt::Size buttonSizeInDips = [this]() {
			return _content.TitleBar().CaptionButtons().CaptionButtonSize();
		}();

		const float buttonWidthInPixels = buttonSizeInDips.Width * _currentDpi / USER_DEFAULT_SCREEN_DPI;
		const float buttonHeightInPixels = buttonSizeInDips.Height * _currentDpi / USER_DEFAULT_SCREEN_DPI;

		if (cursorPos.y >= buttonHeightInPixels) {
			// 鼠标位于标题按钮下方，如果标题栏很宽，这里也可以拖动
			return HTCAPTION;
		}

		// 从右向左检查鼠标是否位于某个标题栏按钮上
		const LONG cursorToRight = titleBarClientRect.right - cursorPos.x;
		if (cursorToRight < buttonWidthInPixels) {
			return HTCLOSE;
		} else if (cursorToRight < buttonWidthInPixels * 2) {
			// 支持 Win11 的贴靠布局
			// FIXME: 最大化时贴靠布局的位置不对，目前没有找到解决方案。似乎只适配了系统原生框架和 UWP
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
		ClientToScreen(_hwndTitleBar, &cursorPos);
		wParam = SendMessage(_hwndTitleBar, WM_NCHITTEST, 0, MAKELPARAM(cursorPos.x, cursorPos.y));
	}
	[[fallthrough]];
	case WM_NCMOUSEMOVE:
	{
		auto captionButtons = _content.TitleBar().CaptionButtons();

		// 将 hover 状态通知 CaptionButtons。标题栏窗口拦截了 XAML Islands 中的标题栏
		// 控件的鼠标消息，标题栏按钮的状态由我们手动控制。
		switch (wParam) {
		case HTTOP:
		case HTCAPTION:
		{
			captionButtons.LeaveButtons();

			// 将 HTTOP 传给主窗口才能通过上边框调整窗口高度
			return SendMessage(_hWnd, msg, wParam, lParam);
		}
		case HTMINBUTTON:
		case HTMAXBUTTON:
		case HTCLOSE:
			captionButtons.HoverButton((winrt::Magpie::App::CaptionButton)wParam);

			// 追踪鼠标以确保鼠标离开标题栏时我们能收到 WM_NCMOUSELEAVE 消息，否则无法
			// 可靠的收到这个消息，尤其是在用户快速移动鼠标的时候。
			if (!_trackingMouse && msg == WM_NCMOUSEMOVE) {
				TRACKMOUSEEVENT ev{};
				ev.cbSize = sizeof(TRACKMOUSEEVENT);
				ev.dwFlags = TME_LEAVE | TME_NONCLIENT;
				ev.hwndTrack = _hwndTitleBar;
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
		// 比如：鼠标在关闭按钮上停留了一段时间，系统会显示文字提示，这时按下左键，便会收
		// 到 WM_NCMOUSELEAVE，但此时鼠标并没有离开标题栏按钮
		POINT cursorPos;
		GetCursorPos(&cursorPos);
		// 先检查鼠标是否在主窗口上，如果正在显示文字提示，会返回 _hwndTitleBar
		HWND hwndUnderCursor = WindowFromPoint(cursorPos);
		if (hwndUnderCursor != _hWnd && hwndUnderCursor != _hwndTitleBar) {
			_content.TitleBar().CaptionButtons().LeaveButtons();
		} else {
			// 然后检查鼠标在标题栏上的位置
			LRESULT hit = SendMessage(_hwndTitleBar, WM_NCHITTEST, 0, MAKELPARAM(cursorPos.x, cursorPos.y));
			if (hit != HTMINBUTTON && hit != HTMAXBUTTON && hit != HTCLOSE) {
				_content.TitleBar().CaptionButtons().LeaveButtons();
			}
		}

		_trackingMouse = false;
		break;
	}
	case WM_NCLBUTTONDOWN:
	case WM_NCLBUTTONDBLCLK:
	{
		// 手动处理标题栏上的点击。如果在标题栏按钮上，则通知 CaptionButtons，否则将消息传递
		// 给主窗口。
		switch (wParam) {
		case HTTOP:
		case HTCAPTION:
		{
			// 将 HTTOP 传给主窗口才能通过上边框调整窗口高度
			return SendMessage(_hWnd, msg, wParam, lParam);
		}
		case HTMINBUTTON:
		case HTMAXBUTTON:
		case HTCLOSE:
			_content.TitleBar().CaptionButtons().PressButton((winrt::Magpie::App::CaptionButton)wParam);
			// 在标题栏按钮上按下左键后我们便捕获光标，这样才能在释放时得到通知。注意捕获光标后
			// 便不会再收到 NC 族消息，这就是为什么我们要处理 WM_MOUSEMOVE 和 WM_LBUTTONUP
			SetCapture(_hwndTitleBar);
			break;
		}
		return 0;
	}
	// 在捕获光标时会收到
	case WM_LBUTTONUP:
	{
		ReleaseCapture();

		POINT cursorPos{ GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam) };
		ClientToScreen(_hwndTitleBar, &cursorPos);
		wParam = SendMessage(_hwndTitleBar, WM_NCHITTEST, 0, MAKELPARAM(cursorPos.x, cursorPos.y));
	}
	[[fallthrough]];
	case WM_NCLBUTTONUP:
	{
		// 处理鼠标在标题栏上释放。如果位于标题栏按钮上，则传递给 CaptionButtons，不在则将消息传递给主窗口
		switch (wParam) {
		case HTTOP:
		case HTCAPTION:
		{
			// 在可拖拽区域或上边框释放左键，将此消息传递给主窗口
			_content.TitleBar().CaptionButtons().ReleaseButtons();
			return SendMessage(_hWnd, msg, wParam, lParam);
		}
		case HTMINBUTTON:
		case HTMAXBUTTON:
		case HTCLOSE:
			// 在标题栏按钮上释放左键
			_content.TitleBar().CaptionButtons().ReleaseButton((winrt::Magpie::App::CaptionButton)wParam);
			break;
		default:
			_content.TitleBar().CaptionButtons().ReleaseButtons();
		}
		
		return 0;
	}
	case WM_NCRBUTTONDOWN:
	case WM_NCRBUTTONDBLCLK:
	case WM_NCRBUTTONUP:
		// 不关心右键，将它们传递给主窗口
		return SendMessage(_hWnd, msg, wParam, lParam);
	}

	return DefWindowProc(_hwndTitleBar, msg, wParam, lParam);
}

void MainWindow::_ResizeTitleBarWindow() noexcept {
	if (!_hwndTitleBar) {
		return;
	}

	auto titleBar = _content.TitleBar();

	// 获取标题栏的边框矩形
	winrt::Rect rect{0.0f, 0.0f, (float)titleBar.ActualWidth(), (float)titleBar.ActualHeight()};
	rect = titleBar.TransformToVisual(_content).TransformBounds(rect);

	const float dpiScale = _currentDpi / float(USER_DEFAULT_SCREEN_DPI);

	// 将标题栏窗口置于 XAML Islands 窗口上方
	SetWindowPos(
		_hwndTitleBar,
		HWND_TOP,
		(int)std::floorf(rect.X * dpiScale),
		(int)std::floorf(rect.Y * dpiScale) + _GetTopBorderHeight(),
		(int)std::ceilf(rect.Width * dpiScale),
		(int)std::floorf(rect.Height * dpiScale + 1),	// 不知为何，直接向上取整有时无法遮盖 TitleBarControl
		SWP_SHOWWINDOW
	);

	// 设置标题栏窗口的最大化样式，这样才能展示正确的文字提示
	LONG_PTR style = GetWindowLongPtr(_hwndTitleBar, GWL_STYLE);
	SetWindowLongPtr(_hwndTitleBar, GWL_STYLE,
		_isMaximized ? style | WS_MAXIMIZE : style & ~WS_MAXIMIZE);
}

}
