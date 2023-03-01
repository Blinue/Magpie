#pragma once
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>
#include <CoreWindow.h>
#include "XamlUtils.h"

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
		case WM_SHOWWINDOW:
		{
			if (wParam == TRUE) {
				SetFocus(_hWnd);
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

			break;
		}
		case WM_MOVING:
		{
			if (_hwndXamlIsland) {
				XamlUtils::RepositionXamlPopups(_content.XamlRoot(), false);
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
		SetWindowPos(_hwndXamlIsland, NULL, 0, 0, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, SWP_SHOWWINDOW | SWP_NOACTIVATE);
	}

	winrt::event<winrt::delegate<>> _destroyedEvent;

	HWND _hwndXamlIsland = NULL;
	winrt::DesktopWindowXamlSource _xamlSource{ nullptr };
	winrt::com_ptr<IDesktopWindowXamlSourceNative2> _xamlSourceNative2;
};

}
