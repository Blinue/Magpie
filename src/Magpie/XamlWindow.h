#pragma once
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>
#include <CoreWindow.h>
#include "XamlUtils.h"
#include "Win32Utils.h"
#include "ThemeHelper.h"
#include "CommonSharedConstants.h"

#pragma comment(lib, "uxtheme.lib")

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
	}

	void _SetTheme(bool isDarkTheme) noexcept {
		_isDarkTheme = isDarkTheme;

		// Win10 中即使在亮色主题下我们也使用暗色边框，这也是 UWP 窗口的行为
		ThemeHelper::SetWindowTheme(
			_hWnd,
			Win32Utils::GetOSVersion().IsWin11() ? isDarkTheme : true,
			isDarkTheme
		);

		if (Win32Utils::GetOSVersion().Is22H2OrNewer()) {
			// 设置 Mica 背景
			DWM_SYSTEMBACKDROP_TYPE value = DWMSBT_MAINWINDOW;
			DwmSetWindowAttribute(_hWnd, DWMWA_SYSTEMBACKDROP_TYPE, &value, sizeof(value));
			return;
		}
		
		if (Win32Utils::GetOSVersion().IsWin11()) {
			// Win11 21H1/21H2 对 Mica 的支持不完善，改为使用纯色背景。Win10 在 WM_PAINT 中
			// 绘制背景。背景色在更改窗口大小时会短暂可见。
			HBRUSH hbrOld = (HBRUSH)SetClassLongPtr(
				_hWnd,
				GCLP_HBRBACKGROUND,
				(INT_PTR)CreateSolidBrush(isDarkTheme ?
				CommonSharedConstants::DARK_TINT_COLOR : CommonSharedConstants::LIGHT_TINT_COLOR));
			if (hbrOld) {
				DeleteObject(hbrOld);
			}
		}

		// 立即重新绘制
		InvalidateRect(_hWnd, nullptr, FALSE);
		UpdateWindow(_hWnd);
	}

	LRESULT _MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
		switch (msg) {
		case WM_CREATE:
		{
			_currentDpi = GetDpiForWindow(_hWnd);

			_UpdateFrameMargins();

			if (!Win32Utils::GetOSVersion().IsWin11()) {
				// 初始化双缓冲绘图
				static const int _ = []() {
					BufferedPaintInit();
					return 0;
				}();
			}

			break;
		}
		case WM_NCCALCSIZE:
		{
			// 移除标题栏的逻辑基本来自 Windows Terminal
			// https://github.com/microsoft/terminal/blob/0ee2c74cd432eda153f3f3e77588164cde95044f/src/cascadia/WindowsTerminal/NonClientIslandWindow.cpp

			if (!wParam) {
				return 0;
			}

			NCCALCSIZE_PARAMS* params = (NCCALCSIZE_PARAMS*)lParam;
			RECT& clientRect = params->rgrc[0];

			// 保存原始上边框位置
			const LONG originalTop = clientRect.top;

			// 应用默认边框
			LRESULT ret = DefWindowProc(_hWnd, WM_NCCALCSIZE, wParam, lParam);
			if (ret != 0) {
				return ret;
			}

			// 重新应用原始上边框，因此我们完全移除了默认边框中的上边框和标题栏，但保留了其他方向的边框
			clientRect.top = originalTop;

			// WM_NCCALCSIZE 在 WM_SIZE 前
			_UpdateMaximizedState();
			
			if (_isMaximized) {
				// 最大化的窗口的实际尺寸比屏幕的工作区更大一点，这是为了将可调整窗口大小的区域隐藏在屏幕外面
				clientRect.top += _GetResizeHandleHeight();

				// 如果有自动隐藏的任务栏，我们在它的方向稍微减小客户区，这样用户就可以用鼠标呼出任务栏
				if (HMONITOR hMon = MonitorFromWindow(_hWnd, MONITOR_DEFAULTTONEAREST)) {
					MONITORINFO monInfo{};
					monInfo.cbSize = sizeof(MONITORINFO);
					GetMonitorInfo(hMon, &monInfo);

					// 检查是否有自动隐藏的任务栏
					APPBARDATA appBarData{};
					appBarData.cbSize = sizeof(appBarData);
					if (SHAppBarMessage(ABM_GETSTATE, &appBarData) & ABS_AUTOHIDE) {
						// 检查显示器的一条边
						auto hasAutohideTaskbar = [&monInfo](UINT edge) -> bool {
							APPBARDATA data{};
							data.cbSize = sizeof(data);
							data.uEdge = edge;
							data.rc = monInfo.rcMonitor;
							HWND hTaskbar = (HWND)SHAppBarMessage(ABM_GETAUTOHIDEBAREX, &data);
							return hTaskbar != nullptr;
						};

						static constexpr int AUTO_HIDE_TASKBAR_HEIGHT = 2;

						if (hasAutohideTaskbar(ABE_TOP)) {
							clientRect.top += AUTO_HIDE_TASKBAR_HEIGHT;
						}
						if (hasAutohideTaskbar(ABE_BOTTOM)) {
							clientRect.bottom -= AUTO_HIDE_TASKBAR_HEIGHT;
						}
						if (hasAutohideTaskbar(ABE_LEFT)) {
							clientRect.left += AUTO_HIDE_TASKBAR_HEIGHT;
						}
						if (hasAutohideTaskbar(ABE_RIGHT)) {
							clientRect.right -= AUTO_HIDE_TASKBAR_HEIGHT;
						}
					}
				}
			}

			return 0;
		}
		case WM_NCHITTEST:
		{
			// 让 OS 处理左右下三边，由于我们移除了标题栏，上边框会被视为客户区
			LRESULT originalRet = DefWindowProc(_hWnd, WM_NCHITTEST, 0, lParam);
			if (originalRet != HTCLIENT) {
				return originalRet;
			}

			// XAML Islands 和它上面的标题栏窗口都会吞掉鼠标事件，因此能到达这里的唯一机会
			// 是上边框。保险起见做一些额外检查。

			if (!_isMaximized) {
				RECT rcWindow;
				GetWindowRect(_hWnd, &rcWindow);

				if (GET_Y_LPARAM(lParam) < rcWindow.top + _GetResizeHandleHeight()) {
					return HTTOP;
				}
			}

			return HTCAPTION;
		}
		case WM_PAINT:
		{
			if (Win32Utils::GetOSVersion().IsWin11()) {
				break;
			}

			PAINTSTRUCT ps{ 0 };
			HDC hdc = BeginPaint(_hWnd, &ps);
			if (!hdc) {
				return 0;
			}

			const int topBorderHeight = (int)_GetTopBorderHeight();

			// 在顶部绘制黑色实线以显示系统原始边框，见 _UpdateFrameMargins
			if (ps.rcPaint.top < topBorderHeight) {
				RECT rcTopBorder = ps.rcPaint;
				rcTopBorder.bottom = topBorderHeight;
				
				static HBRUSH hBrush = GetStockBrush(BLACK_BRUSH);
				FillRect(hdc, &rcTopBorder, hBrush);
			}

			// 绘制客户区，它会在调整窗口尺寸时短暂可见
			if (ps.rcPaint.bottom > topBorderHeight) {
				RECT rcRest = ps.rcPaint;
				rcRest.top = topBorderHeight;

				static bool isDarkBrush = _isDarkTheme;
				static HBRUSH backgroundBrush = CreateSolidBrush(isDarkBrush ?
					CommonSharedConstants::DARK_TINT_COLOR : CommonSharedConstants::LIGHT_TINT_COLOR);

				if (isDarkBrush != _isDarkTheme) {
					isDarkBrush = _isDarkTheme;
					DeleteBrush(backgroundBrush);
					backgroundBrush = CreateSolidBrush(isDarkBrush ?
						CommonSharedConstants::DARK_TINT_COLOR : CommonSharedConstants::LIGHT_TINT_COLOR);
				}

				if (isDarkBrush) {
					// 这里我们想要黑色背景而不是原始边框
					// hack 来自 https://github.com/microsoft/terminal/blob/0ee2c74cd432eda153f3f3e77588164cde95044f/src/cascadia/WindowsTerminal/NonClientIslandWindow.cpp#L1030-L1047
					HDC opaqueDc;
					BP_PAINTPARAMS params = { sizeof(params), BPPF_NOCLIP | BPPF_ERASE };
					HPAINTBUFFER buf = BeginBufferedPaint(hdc, &rcRest, BPBF_TOPDOWNDIB, &params, &opaqueDc);
					if (buf && opaqueDc) {
						FillRect(opaqueDc, &rcRest, backgroundBrush);
						BufferedPaintSetAlpha(buf, nullptr, 255);
						EndBufferedPaint(buf, TRUE);
					}
				} else {
					FillRect(hdc, &rcRest, backgroundBrush);
				}
			}

			EndPaint(_hWnd, &ps);
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
			_currentDpi = HIWORD(wParam);

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

			_UpdateFrameMargins();

			return 0;
		}
		case WM_DESTROY:
		{
			_hWnd = NULL;

			_xamlSourceNative2 = nullptr;
			// 必须手动重置 Content，否则会内存泄露，使 RootPage 无法析构
			_xamlSource.Content(nullptr);
			_xamlSource.Close();
			_xamlSource = nullptr;
			_hwndXamlIsland = NULL;

			_isMaximized = false;
			_isWindowShown = false;
			_isDarkTheme = false;

			_content = nullptr;

			_destroyedEvent();

			return 0;
		}
		}

		return DefWindowProc(_hWnd, msg, wParam, lParam);
	}

	uint32_t _GetTopBorderHeight() const noexcept {
		static constexpr uint32_t TOP_BORDER_HEIGHT = 1;

		// Win11 或最大化时没有上边框
		return (Win32Utils::GetOSVersion().IsWin11() || _isMaximized) ? 0 : TOP_BORDER_HEIGHT;
	}

	int _GetResizeHandleHeight() noexcept {
		// 没有 SM_CYPADDEDBORDER
		return GetSystemMetricsForDpi(SM_CXPADDEDBORDER, _currentDpi) +
			GetSystemMetricsForDpi(SM_CYSIZEFRAME, _currentDpi);
	}

	HWND _hWnd = NULL;
	C _content{ nullptr };

	uint32_t _currentDpi = USER_DEFAULT_SCREEN_DPI;
	bool _isMaximized = false;
	bool _isWindowShown = false;
	bool _isDarkTheme = false;

private:
	void _UpdateIslandPosition(int width, int height) const noexcept {
		if (!IsWindowVisible(_hWnd) && _isMaximized) {
			// 初始化过程中此函数会被调用两次。如果窗口以最大化显示，则两次传入的尺寸不一致。第一次
			// 调用此函数时主窗口尚未显示，因此无法最大化，我们必须估算最大化窗口的尺寸。不执行这个
			// 操作可能导致窗口显示时展示 NavigationView 导航展开的动画。
			if (HMONITOR hMon = MonitorFromWindow(_hWnd, MONITOR_DEFAULTTONEAREST)) {
				MONITORINFO monInfo{};
				monInfo.cbSize = sizeof(MONITORINFO);
				GetMonitorInfo(hMon, &monInfo);

				// 最大化窗口的尺寸为当前屏幕工作区的尺寸
				width = monInfo.rcWork.right - monInfo.rcMonitor.left;
				height = monInfo.rcWork.bottom - monInfo.rcMonitor.top;
			}
		}

		int topBorderHeight = _GetTopBorderHeight();

		// SWP_NOZORDER 确保 XAML Islands 窗口始终在标题栏窗口下方，否则主窗口在调整大小时会闪烁
		SetWindowPos(
			_hwndXamlIsland,
			NULL,
			0,
			topBorderHeight,
			width,
			height - topBorderHeight,
			SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW
		);
	}

	void _UpdateMaximizedState() noexcept {
		// 如果窗口尚未显示，不碰 _isMaximized
		if (_isWindowShown) {
			_isMaximized = IsMaximized(_hWnd);
		}
	}

	void _UpdateFrameMargins() const noexcept {
		if (Win32Utils::GetOSVersion().IsWin11()) {
			return;
		}

		MARGINS margins{};
		if (_GetTopBorderHeight() > 0) {
			// 在 Win10 中，移除标题栏时上边框也被没了。我们的解决方案是：使用 DwmExtendFrameIntoClientArea
			// 将边框扩展到客户区，然后在顶部绘制了一个黑色实线来显示系统原始边框（这种情况下操作系统将黑色视
			// 为透明）。因此我们有**完美**的上边框！
			// 见 https://docs.microsoft.com/en-us/windows/win32/dwm/customframe#extending-the-client-frame
			// 
			// 有的软件自己绘制了假的上边框，如 Chromium 系、WinUI 3 等，但窗口失去焦点时边框是半透明的，无法
			// 完美模拟。
			//
			// 我们选择扩展到标题栏高度，这是最好的选择。一个自然的想法是，既然上边框只有一个像素高，我们扩展一
			// 个像素即可，可惜因为 DWM 的 bug，这会使窗口失去焦点时上边框变为透明。那么能否传一个负值，让边框
			// 扩展到整个客户区？这大部分情况下可以工作，有一个小 bug：不显示边框颜色的设置下深色模式的边框会变
			// 为纯黑而不是半透明。
			RECT frame{};
			AdjustWindowRectExForDpi(&frame, GetWindowStyle(_hWnd), FALSE, 0, _currentDpi);
			margins.cyTopHeight = -frame.top;
		}
		DwmExtendFrameIntoClientArea(_hWnd, &margins);
	}

	winrt::event<winrt::delegate<>> _destroyedEvent;

	HWND _hwndXamlIsland = NULL;
	winrt::DesktopWindowXamlSource _xamlSource{ nullptr };
	winrt::com_ptr<IDesktopWindowXamlSourceNative2> _xamlSourceNative2;
};

}
