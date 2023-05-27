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

	// 创建标题栏窗口，它是主窗口的子窗口。我们将它至于 XAML Islands 窗口之上以防止鼠标事件被吞掉
	// 
	// 出于未知的原因，必须添加 WS_EX_LAYERED 样式才能发挥作用，见
	// https://github.com/microsoft/terminal/blob/0ee2c74cd432eda153f3f3e77588164cde95044f/src/cascadia/WindowsTerminal/NonClientIslandWindow.cpp#L79
	// WS_EX_NOREDIRECTIONBITMAP 可以避免 WS_EX_LAYERED 导致的额外内存开销
	//
	// WS_MINIMIZEBOX 和 WS_MAXIMIZEBOX 使操作系统以为此窗口有标题栏按钮，如果没有这些样式，鼠标
	// 在标题栏按钮上悬停时没有文字提示。但 Win11 的贴靠布局不依赖它们。
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

	_SetContent(winrt::Magpie::App::MainPage());

	// Xaml 控件加载完成后显示主窗口
	_content.Loaded([this, isMaximized](winrt::IInspectable const&, winrt::RoutedEventArgs const&) -> winrt::IAsyncAction {
		co_await _content.Dispatcher().RunAsync(winrt::CoreDispatcherPriority::Normal, [hWnd(_hWnd), isMaximized]() {
			// 防止窗口显示时背景闪烁
			// https://stackoverflow.com/questions/69715610/how-to-initialize-the-background-color-of-win32-app-to-something-other-than-whit
			SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
			ShowWindow(hWnd, isMaximized ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL);
			Win32Utils::SetForegroundWindow(hWnd);
		});
	});

	_content.ActualThemeChanged([this](winrt::FrameworkElement const&, winrt::IInspectable const&) {
		_UpdateTheme();
	});
	_UpdateTheme();

	_content.SizeChanged([this](winrt::IInspectable const&, winrt::SizeChangedEventArgs const&) {
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
	case WM_DESTROY:
	{
		XamlApp::Get().SaveSettings();
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
		const POINT cursorPos{ GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam) };

		RECT rcParent;
		GetWindowRect(_hWnd, &rcParent);

		if (cursorPos.y < rcParent.top + _GetResizeHandleHeight()) {
			// 鼠标位于上边框
			return HTTOP;
		}

		auto captionButtons = _content.TitleBar().CaptionButtons();
		static const double buttonWidthInDips = captionButtons.CaptionButtonWidth();

		const double buttonWidthInPixels =  buttonWidthInDips * _currentDpi / USER_DEFAULT_SCREEN_DPI;

		RECT clientRect;
		GetClientRect(_hWnd, &clientRect);
		POINT rightTop{ clientRect.right, clientRect.top };
		ClientToScreen(_hWnd, &rightTop);

		// 从右向左检查鼠标是否位于某个标题栏按钮上
		const double cursorToRight = rightTop.x - cursorPos.x;
		if (cursorToRight < buttonWidthInPixels) {
			return HTCLOSE;
		} else if (cursorToRight < buttonWidthInPixels * 2) {
			return HTMAXBUTTON;
		} else if (cursorToRight < buttonWidthInPixels * 3) {
			return HTMINBUTTON;
		} else {
			return HTCAPTION;
		}
	}
	case WM_NCMOUSEMOVE:
		// When we get this message, it's because the mouse moved when it was
		// over somewhere we said was the non-client area.
		//
		// We'll use this to communicate state to the title bar control, so that
		// it can update its visuals.
		// - If we're over a button, hover it.
		// - If we're over _anything else_, stop hovering the buttons.
		switch (wParam) {
		case HTTOP:
		case HTCAPTION:
		{
			//_titlebar.ReleaseButtons();

			// Pass caption-related nonclient messages to the parent window.
			// Make sure to do this for the HTTOP, which is the top resize
			// border, so we can resize the window on the top.
			return SendMessage(_hWnd, msg, wParam, lParam);
		}
		case HTMINBUTTON:
		case HTMAXBUTTON:
		case HTCLOSE:
			//_titlebar.HoverButton(static_cast<winrt::TerminalApp::CaptionButton>(wparam));
			break;
		//default:
			//_titlebar.ReleaseButtons();
		}

		// If we haven't previously asked for mouse tracking, request mouse
		// tracking. We need to do this so we can get the WM_NCMOUSELEAVE
		// message when the mouse leave the titlebar. Otherwise, we won't always
		// get that message (especially if the user moves the mouse _real
		// fast_).
		/*if (!_trackingMouse &&
			(wparam == HTMINBUTTON || wparam == HTMAXBUTTON || wparam == HTCLOSE)) {
			TRACKMOUSEEVENT ev{};
			ev.cbSize = sizeof(TRACKMOUSEEVENT);
			// TME_NONCLIENT is absolutely critical here. In my experimentation,
			// we'd get WM_MOUSELEAVE messages after just a HOVER_DEFAULT
			// timeout even though we're not requesting TME_HOVER, which kinda
			// ruined the whole point of this.
			ev.dwFlags = TME_LEAVE | TME_NONCLIENT;
			ev.hwndTrack = _dragBarWindow.get();
			ev.dwHoverTime = HOVER_DEFAULT; // we don't _really_ care about this.
			LOG_IF_WIN32_BOOL_FALSE(TrackMouseEvent(&ev));
			_trackingMouse = true;
		}*/
		break;

	case WM_NCMOUSELEAVE:
	case WM_MOUSELEAVE:
		// When the mouse leaves the drag rect, make sure to dismiss any hover.
		//_titlebar.ReleaseButtons();
		//_trackingMouse = false;
		break;

	// NB: *Shouldn't be forwarding these* when they're not over the caption
	// because they can inadvertently take action using the system's default
	// metrics instead of our own.
	case WM_NCLBUTTONDOWN:
	case WM_NCLBUTTONDBLCLK:
		// Manual handling for mouse clicks in the drag bar. If it's in a
		// caption button, then tell the titlebar to "press" the button, which
		// should change its visual state.
		//
		// If it's not in a caption button, then just forward the message along
		// to the root HWND. Make sure to do this for the HTTOP, which is the
		// top resize border.
		switch (wParam) {
		case HTTOP:
		case HTCAPTION:
		{
			// Pass caption-related nonclient messages to the parent window.
			return SendMessage(_hWnd, msg, wParam, lParam);
		}
		// The buttons won't work as you'd expect; we need to handle those
		// ourselves.
		case HTMINBUTTON:
		case HTMAXBUTTON:
		case HTCLOSE:
			//_titlebar.PressButton(static_cast<winrt::TerminalApp::CaptionButton>(wparam));
			break;
		}
		return 0;

	case WM_NCLBUTTONUP:
		// Manual handling for mouse RELEASES in the drag bar. If it's in a
		// caption button, then manually handle what we'd expect for that button.
		//
		// If it's not in a caption button, then just forward the message along
		// to the root HWND.
		switch (wParam) {
		case HTTOP:
		case HTCAPTION:
		{
			// Pass caption-related nonclient messages to the parent window.
			// The buttons won't work as you'd expect; we need to handle those ourselves.
			return SendMessage(_hWnd, msg, wParam, lParam);
		}
		break;

		// If we do find a button, then tell the titlebar to raise the same
		// event that would be raised if it were "tapped"
		case HTMINBUTTON:
		case HTMAXBUTTON:
		case HTCLOSE:
			//_titlebar.ReleaseButtons();
			//_titlebar.ClickButton(static_cast<winrt::TerminalApp::CaptionButton>(wparam));
			break;
		}
		return 0;

	// Make sure to pass along right-clicks in this region to our parent window
	// - we don't need to handle these.
	case WM_NCRBUTTONDOWN:
	case WM_NCRBUTTONDBLCLK:
	case WM_NCRBUTTONUP:
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
}

}
