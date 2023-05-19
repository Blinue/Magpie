#pragma once
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>
#include <CoreWindow.h>
#include "XamlUtils.h"

static constexpr int AUTO_HIDE_TASKBAR_HEIGHT = 2;
static constexpr int TOP_BORDER_HEIGHT = 1;

namespace Magpie {

template <typename T, typename C>
class XamlWindowT {
public:
	virtual ~XamlWindowT() {
		if (_hWnd) {
			DestroyWindow(_hWnd);
		}
	}

	operator bool() const noexcept {
		return _hWnd;
	}

	void HandleMessage(const MSG& msg) {
		// XAML Islands 会吞掉 Alt+F4，需要特殊处理
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

	HWND Handle() const noexcept {
		return _hWnd;
	}

	const C& Content() const noexcept {
		return _content;
	}

	void Destroy() {
		DestroyWindow(_hWnd);
	}

	winrt::event_token Destroyed(winrt::delegate<> const& handler) {
		return _destroyedEvent.add(handler);
	}

protected:
	using base_type = XamlWindowT<T, C>;

	static LRESULT CALLBACK _WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
		if (msg == WM_NCCREATE) {
			XamlWindowT* that = (XamlWindowT*)(((CREATESTRUCT*)lParam)->lpCreateParams);
			assert(that && !that->_hWnd);
			that->_hWnd = hWnd;
			SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)that);
		} else if (T* that = (T*)GetWindowLongPtr(hWnd, GWLP_USERDATA)) {
			return that->_MessageHandler(msg, wParam, lParam);
		}

		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	void _SetContent(C const& content) {
		_content = content;

		// 初始化 XAML Islands
		_xamlSource = winrt::DesktopWindowXamlSource();
		_xamlSourceNative2 = _xamlSource.as<IDesktopWindowXamlSourceNative2>();

		auto interop = _xamlSource.as<IDesktopWindowXamlSourceNative>();
		interop->AttachToWindow(_hWnd);
		interop->get_WindowHandle(&_hwndXamlIsland);
		_xamlSource.Content(content);

		// 焦点始终位于 _hwndXamlIsland 中
		_xamlSource.TakeFocusRequested(
			[](winrt::DesktopWindowXamlSource const& sender,
			winrt::DesktopWindowXamlSourceTakeFocusRequestedEventArgs const& args
		) {
			winrt::XamlSourceFocusNavigationReason reason = args.Request().Reason();
			if (reason < winrt::XamlSourceFocusNavigationReason::Left) {
				sender.NavigateFocus(args.Request());
			}
		});

		// 防止第一次收到 WM_SIZE 消息时 MainPage 尺寸为 0
		RECT windowRect;
		GetWindowRect(_hWnd, &windowRect);
		_UpdateIslandPosition(windowRect.right - windowRect.left, windowRect.bottom - windowRect.top);
	}

	LRESULT _MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
		switch (msg) {
		case WM_CREATE:
		{
			_accentColor = _GetAccentColor();
			return 0;
		}
		case WM_NCCALCSIZE:
		{
			// 移除标题栏的逻辑基本来自 Windows Terminal
			// https://github.com/microsoft/terminal/blob/0ee2c74cd432eda153f3f3e77588164cde95044f/src/cascadia/WindowsTerminal/NonClientIslandWindow.cpp

			if (!wParam) {
				return 0;
			}

			auto params = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);

			// Store the original top before the default window proc applies the
			// default frame.
			const auto originalTop = params->rgrc[0].top;

			const auto originalSize = params->rgrc[0];

			// apply the default frame
			const auto ret = DefWindowProc(_hWnd, WM_NCCALCSIZE, wParam, lParam);
			if (ret != 0) {
				return ret;
			}

			auto newSize = params->rgrc[0];
			// Re-apply the original top from before the size of the default frame was applied.
			newSize.top = originalTop;

			// WM_NCCALCSIZE 在 WM_SIZE 前
			_UpdateMaximizedState();

			// We don't need this correction when we're fullscreen. We will have the
			// WS_POPUP size, so we don't have to worry about borders, and the default
			// frame will be fine.
			if (_isMaximized) {
				// When a window is maximized, its size is actually a little bit more
				// than the monitor's work area. The window is positioned and sized in
				// such a way that the resize handles are outside of the monitor and
				// then the window is clipped to the monitor so that the resize handle
				// do not appear because you don't need them (because you can't resize
				// a window when it's maximized unless you restore it).
			    newSize.top += _GetResizeHandleHeight(GetDpiForWindow(_hWnd));
			}

			// GH#1438 - Attempt to detect if there's an autohide taskbar, and if there
			// is, reduce our size a bit on the side with the taskbar, so the user can
			// still mouse-over the taskbar to reveal it.
			// GH#5209 - make sure to use MONITOR_DEFAULTTONEAREST, so that this will
			// still find the right monitor even when we're restoring from minimized.
			auto hMon = MonitorFromWindow(_hWnd, MONITOR_DEFAULTTONEAREST);
			if (hMon && _isMaximized) {
				MONITORINFO monInfo{};
				monInfo.cbSize = sizeof(MONITORINFO);
				GetMonitorInfo(hMon, &monInfo);

				// First, check if we have an auto-hide taskbar at all:
				APPBARDATA autohide{ 0 };
				autohide.cbSize = sizeof(autohide);
				if (SHAppBarMessage(ABM_GETSTATE, &autohide) & ABS_AUTOHIDE) {
					// This helper can be used to determine if there's a auto-hide
					// taskbar on the given edge of the monitor we're currently on.
					auto hasAutohideTaskbar = [&monInfo](UINT edge) -> bool {
						APPBARDATA data{ 0 };
						data.cbSize = sizeof(data);
						data.uEdge = edge;
						data.rc = monInfo.rcMonitor;
						auto hTaskbar = (HWND)SHAppBarMessage(ABM_GETAUTOHIDEBAREX, &data);
						return hTaskbar != nullptr;
					};

					bool onTop = hasAutohideTaskbar(ABE_TOP);
					bool onBottom = hasAutohideTaskbar(ABE_BOTTOM);
					bool onLeft = hasAutohideTaskbar(ABE_LEFT);
					bool onRight = hasAutohideTaskbar(ABE_RIGHT);

					// If there's a taskbar on any side of the monitor, reduce our size
					// a little bit on that edge.
					//
					// Note to future code archeologists:
					// This doesn't seem to work for fullscreen on the primary display.
					// However, testing a bunch of other apps with fullscreen modes
					// and an auto-hiding taskbar has shown that _none_ of them
					// reveal the taskbar from fullscreen mode. This includes Edge,
					// Firefox, Chrome, Sublime Text, PowerPoint - none seemed to
					// support this.
					//
					// This does however work fine for maximized.
					if (onTop) {
						// Peculiarly, when we're fullscreen,
						newSize.top += AUTO_HIDE_TASKBAR_HEIGHT;
					}
					if (onBottom) {
						newSize.bottom -= AUTO_HIDE_TASKBAR_HEIGHT;
					}
					if (onLeft) {
						newSize.left += AUTO_HIDE_TASKBAR_HEIGHT;
					}
					if (onRight) {
						newSize.right -= AUTO_HIDE_TASKBAR_HEIGHT;
					}
				}
			}

			params->rgrc[0] = newSize;

			return 0;
		}
		case WM_NCHITTEST:
		{
			// 操作系统无法处理上边框，因为我们移除了标题栏，上边框被视为客户区
			LRESULT originalRet = DefWindowProc(_hWnd, WM_NCHITTEST, 0, lParam);
			if (originalRet != HTCLIENT) {
				return originalRet;
			}

			// At this point, we know that the cursor is inside the client area so it
			// has to be either the little border at the top of our custom title bar,
			// the drag bar or something else in the XAML island. But the XAML Island
			// handles WM_NCHITTEST on its own so actually it cannot be the XAML
			// Island. Then it must be the drag bar or the little border at the top
			// which the user can use to move or resize the window.

			if (!_isMaximized) {
				// the top of the drag bar is used to resize the window
				RECT rcWindow;
				GetWindowRect(_hWnd, &rcWindow);

				int resizeBorderHeight = _GetResizeHandleHeight(GetDpiForWindow(_hWnd));

				if (GET_Y_LPARAM(lParam) < rcWindow.top + resizeBorderHeight) {
					return HTTOP;
				}
			}

			return HTCAPTION;
		}
		case WM_PAINT:
		{
			// 移除标题栏时上边框也被没了，因此我们自己绘制一个假的。这也是很多软件的解决方案，如 Chromium 系、WinUI 3 等。
			// 注意 Windows Terminal 似乎也绘制了上边框，但和我们原理不同。它使用 DwmExtendFrameIntoClientArea 将边框扩
			// 展到窗口内部，然后在顶部绘制了一个黑色实线来显示系统原始边框（这种情况下操作系统将黑色视为透明）。这种方法可
			// 以获得完美的上边框，但有一个很大的弊端：调整大小时会露出原始标题栏，十分丑陋。
			//
			// 我们的上边框几乎可以以假乱真，只有失去焦点时无法完美模拟，因为此时窗口边框是半透明的。

			PAINTSTRUCT ps{ 0 };
			HDC hdc = BeginPaint(_hWnd, &ps);
			if (!hdc) {
				return 0;
			}

			if (ps.rcPaint.top < TOP_BORDER_HEIGHT) {
				auto rcTopBorder = ps.rcPaint;
				rcTopBorder.bottom = TOP_BORDER_HEIGHT;

				HBRUSH hBrush = CreateSolidBrush(_accentColor);
				FillRect(hdc, &rcTopBorder, hBrush);
				DeleteObject(hBrush);
			}

			EndPaint(_hWnd, &ps);
			return 0;
		}
		case WM_DWMCOLORIZATIONCOLORCHANGED:
		{
			DWORD color = 0;
			BOOL opaque = FALSE;
			DwmGetColorizationColor(&color, &opaque);
			COLORREF newAccentColor = RGB((color & 0x00ff0000) >> 16, (color & 0x0000ff00) >> 8, color & 0x000000ff);

			// 主题色改变时我们会收到多个 WM_DWMCOLORIZATIONCOLORCHANGED，只需处理一次
			if (newAccentColor != _accentColor) {
				_accentColor = newAccentColor;

				RECT rect;
				GetClientRect(_hWnd, &rect);
				rect.bottom = TOP_BORDER_HEIGHT;
				InvalidateRect(_hWnd, &rect, FALSE);

				// 立即重新绘制以避免闪烁
				UpdateWindow(_hWnd);
			}

			return 0;
		}
		case WM_SHOWWINDOW:
		{
			if (wParam == TRUE) {
				// 将焦点置于 XAML Islands 窗口可以修复按 Alt 键会导致 UI 无法交互的问题
				SetFocus(_hwndXamlIsland);
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
			break;
		}
		case WM_DPICHANGED:
		{
			RECT* newRect = (RECT*)lParam;
			SetWindowPos(_hWnd,
				NULL,
				newRect->left,
				newRect->top,
				newRect->right - newRect->left,
				newRect->bottom - newRect->top,
				SWP_NOZORDER | SWP_NOACTIVATE
			);

			return 0;
		}
		case WM_MOVING:
		{
			if (_hwndXamlIsland) {
				XamlUtils::RepositionXamlPopups(_content.XamlRoot(), false);
			}

			return 0;
		}
		case WM_MENUCHAR:
		{
			// 防止按 Alt+Key 时发出铃声
			return MAKELRESULT(0, MNC_CLOSE);
		}
		case WM_SYSCOMMAND:
		{
			// 最小化时关闭 ComboBox
			// 不能在 WM_SIZE 中处理，该消息发送于最小化之后，会导致 ComboBox 无法交互
			if (wParam == SC_MINIMIZE && _content) {
				XamlUtils::CloseXamlPopups(_content.XamlRoot());
			}

			break;
		}
		case WM_ACTIVATE:
		{
			if (_hwndXamlIsland) {
				if (LOWORD(wParam) != WA_INACTIVE) {
					SetFocus(_hwndXamlIsland);
				} else {
					XamlUtils::CloseXamlPopups(_content.XamlRoot());
				}
			}

			return 0;
		}
		case WM_SIZE:
		{
			_UpdateMaximizedState();

			if (wParam != SIZE_MINIMIZED) {
				_UpdateIslandPosition(LOWORD(lParam), HIWORD(lParam));

				if (_hwndXamlIsland) {
					// 使 ContentDialog 跟随窗口尺寸调整
					// 来自 https://github.com/microsoft/microsoft-ui-xaml/issues/3577#issuecomment-1399250405
					if (winrt::CoreWindow coreWindow = winrt::CoreWindow::GetForCurrentThread()) {
						HWND hwndDWXS;
						coreWindow.as<ICoreWindowInterop>()->get_WindowHandle(&hwndDWXS);
						PostMessage(hwndDWXS, WM_SIZE, wParam, lParam);
					}

					[](C const& content)->winrt::fire_and_forget {
						co_await content.Dispatcher().RunAsync(winrt::CoreDispatcherPriority::Normal, [xamlRoot(content.XamlRoot())]() {
							XamlUtils::RepositionXamlPopups(xamlRoot, true);
						});
					}(_content);
				}
			}

			return 0;
		}
		case WM_DESTROY:
		{
			_hWnd = NULL;

			_xamlSourceNative2 = nullptr;
			// 必须手动重置 Content，否则会内存泄露，使 MainPage 无法析构
			_xamlSource.Content(nullptr);
			_xamlSource.Close();
			_xamlSource = nullptr;
			_hwndXamlIsland = NULL;

			_content = nullptr;

			_destroyedEvent();

			return 0;
		}
		}

		return DefWindowProc(_hWnd, msg, wParam, lParam);
	}

	HWND _hWnd = NULL;
	C _content{ nullptr };

private:
	void _UpdateIslandPosition(int width, int height) const noexcept {
		// 在顶部保留了上边框空间
		SetWindowPos(_hwndXamlIsland, NULL, 0, TOP_BORDER_HEIGHT, width, height - TOP_BORDER_HEIGHT,
			SWP_SHOWWINDOW | SWP_NOACTIVATE);
	}

	void _UpdateMaximizedState() noexcept {
		_isMaximized = IsMaximized(_hWnd);
	}

	int _GetTopBorderHeight() const noexcept {
		// 最大化时没有上边框
		return _isMaximized ? 0 : TOP_BORDER_HEIGHT;
	}

	static int _GetResizeHandleHeight(UINT dpi) noexcept {
		// 没有 SM_CYPADDEDBORDER
		return GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi) +
			GetSystemMetricsForDpi(SM_CYSIZEFRAME, dpi);
	}

	static COLORREF _GetAccentColor() noexcept {
		DWORD color = 0;
		BOOL opaque = FALSE;
		DwmGetColorizationColor(&color, &opaque);
		return RGB((color & 0x00ff0000) >> 16, (color & 0x0000ff00) >> 8, color & 0x000000ff);
	}

	winrt::event<winrt::delegate<>> _destroyedEvent;

	HWND _hwndXamlIsland = NULL;
	winrt::DesktopWindowXamlSource _xamlSource{ nullptr };
	winrt::com_ptr<IDesktopWindowXamlSourceNative2> _xamlSourceNative2;

	COLORREF _accentColor = 0;
	bool _isMaximized = false;
};

}
