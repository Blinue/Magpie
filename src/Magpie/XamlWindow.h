#pragma once
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>
#include <CoreWindow.h>
#include "XamlUtils.h"

static constexpr int AutohideTaskbarSize = 2;

constexpr int TOP_BORDER_HEIGHT = 1;



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
		_OnResize();
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

			// WM_NCCALCSIZE is called before WM_SIZE
			// _UpdateMaximizedState();

			// We don't need this correction when we're fullscreen. We will have the
			// WS_POPUP size, so we don't have to worry about borders, and the default
			// frame will be fine.
			//if (_isMaximized && !_fullscreen) {
				// When a window is maximized, its size is actually a little bit more
				// than the monitor's work area. The window is positioned and sized in
				// such a way that the resize handles are outside of the monitor and
				// then the window is clipped to the monitor so that the resize handle
				// do not appear because you don't need them (because you can't resize
				// a window when it's maximized unless you restore it).
			//    newSize.top += _GetResizeHandleHeight();
			//}

			// GH#1438 - Attempt to detect if there's an autohide taskbar, and if there
			// is, reduce our size a bit on the side with the taskbar, so the user can
			// still mouse-over the taskbar to reveal it.
			// GH#5209 - make sure to use MONITOR_DEFAULTTONEAREST, so that this will
			// still find the right monitor even when we're restoring from minimized.

			params->rgrc[0] = newSize;

			return 0;
		}
		case WM_NCHITTEST:
		{
			// This will handle the left, right and bottom parts of the frame because
			// we didn't change them.
			const auto originalRet = DefWindowProc(_hWnd, WM_NCHITTEST, 0, lParam);

			if (originalRet != HTCLIENT) {
				// If we're the quake window, suppress resizing on any side except the
				// bottom. I don't believe that this actually works on the top. That's
				// handled below.
				return originalRet;
			}

			// At this point, we know that the cursor is inside the client area so it
			// has to be either the little border at the top of our custom title bar,
			// the drag bar or something else in the XAML island. But the XAML Island
			// handles WM_NCHITTEST on its own so actually it cannot be the XAML
			// Island. Then it must be the drag bar or the little border at the top
			// which the user can use to move or resize the window.

			RECT rcWindow;
			GetWindowRect(_hWnd, &rcWindow);

			const auto resizeBorderHeight = _GetResizeHandleHeight(GetDpiForWindow(_hWnd));
			const auto isOnResizeBorder = GET_Y_LPARAM(lParam) < rcWindow.top + resizeBorderHeight;

			// the top of the drag bar is used to resize the window
			if (isOnResizeBorder) {
				// However, if we're the quake window, then just return HTCAPTION so we
				// don't get a resize handle on the top.
				return HTTOP;
			}

			return HTCAPTION;
		}
		case WM_PAINT:
		{
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
			break;
		}
		case WM_DWMCOLORIZATIONCOLORCHANGED:
		{
			DWORD color = 0;
			BOOL opaque = FALSE;
			DwmGetColorizationColor(&color, &opaque);
			COLORREF newAccentColor = RGB((color & 0x00ff0000) >> 16, (color & 0x0000ff00) >> 8, color & 0x000000ff);
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
			if (wParam != SIZE_MINIMIZED) {
				_OnResize();

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
	void _OnResize() noexcept {
		RECT clientRect;
		GetClientRect(_hWnd, &clientRect);
		SetWindowPos(_hwndXamlIsland, NULL, 0, 1, clientRect.right - clientRect.left,
			clientRect.bottom - clientRect.top - 1, SWP_SHOWWINDOW | SWP_NOACTIVATE);
	}

	static int _GetResizeHandleHeight(UINT dpi) noexcept {
		// there isn't a SM_CYPADDEDBORDER for the Y axis
		return ::GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi) +
			::GetSystemMetricsForDpi(SM_CYSIZEFRAME, dpi);
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
};

}
