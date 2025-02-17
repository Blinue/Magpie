#pragma once
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include <CoreWindow.h>
#include "XamlHelper.h"
#include "Win32Helper.h"
#include "ThemeHelper.h"
#include "Logger.h"
#include "Event.h"
#include "WindowBase.h"
#include <dwmapi.h>

namespace Magpie {

template <typename T, typename C>
class XamlWindowT : public WindowBaseT<T> {
	using base_type = WindowBaseT<T>;
public:
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

	const C& Content() const noexcept {
		return _content;
	}

	uint32_t CurrentDpi() const noexcept {
		return _currentDpi;
	}

	Event<uint32_t> DpiChanged;
	Event<> Destroyed;

protected:
	~XamlWindowT() = default;

	void _Content(C const& content) {
		using namespace winrt::Windows::UI::Xaml::Hosting;

		_content = content;

		// 初始化 XAML Islands
		_xamlSource = DesktopWindowXamlSource();
		_xamlSourceNative2 = _xamlSource.as<IDesktopWindowXamlSourceNative2>();
		_xamlSourceNative2->AttachToWindow(this->Handle());
		_xamlSourceNative2->get_WindowHandle(&_hwndXamlIsland);
		_xamlSource.Content(*content);

		// 焦点始终位于 _hwndXamlIsland 中
		_xamlSource.TakeFocusRequested(
			[](DesktopWindowXamlSource const& sender,
			DesktopWindowXamlSourceTakeFocusRequestedEventArgs const& args
		) {
			XamlSourceFocusNavigationReason reason = args.Request().Reason();
			if (reason < XamlSourceFocusNavigationReason::Left) {
				sender.NavigateFocus(args.Request());
			}
		});
	}

	bool _IsMaximized() const noexcept {
		return _isMaximized;
	}

	void _SetTheme(bool isLightTheme) noexcept {
		_isLightTheme = isLightTheme;

		const HWND hWnd = this->Handle();

		// Win10 中即使在亮色主题下我们也使用暗色边框，这也是 UWP 窗口的行为
		ThemeHelper::SetWindowTheme(
			hWnd,
			Win32Helper::GetOSVersion().IsWin11() ? !isLightTheme : true,
			!isLightTheme
		);

		if (Win32Helper::GetOSVersion().Is22H2OrNewer()) {
			// 设置 Mica 背景
			DWM_SYSTEMBACKDROP_TYPE value = DWMSBT_MAINWINDOW;
			DwmSetWindowAttribute(hWnd, DWMWA_SYSTEMBACKDROP_TYPE, &value, sizeof(value));
			return;
		}
		
		if (Win32Helper::GetOSVersion().IsWin11()) {
			// Win11 21H1/21H2 对 Mica 的支持不完善，改为使用纯色背景。Win10 在 WM_PAINT 中
			// 绘制背景。背景色在更改窗口大小时会短暂可见。
			HBRUSH hbrOld = (HBRUSH)SetClassLongPtr(
				hWnd,
				GCLP_HBRBACKGROUND,
				(INT_PTR)CreateSolidBrush(isLightTheme ? ThemeHelper::LIGHT_TINT_COLOR : ThemeHelper::DARK_TINT_COLOR)
			);
			if (hbrOld) {
				DeleteObject(hbrOld);
			}
		}

		// 立即重新绘制
		InvalidateRect(hWnd, nullptr, FALSE);
		UpdateWindow(hWnd);
	}

	LRESULT _MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
		switch (msg) {
		case WM_CREATE:
		{
			_UpdateDpi(GetDpiForWindow(this->Handle()));

			if (!Win32Helper::GetOSVersion().IsWin11()) {
				// 初始化双缓冲绘图
				static Ignore _ = []() {
					BufferedPaintInit();
					return Ignore();
				}();

				_UpdateFrameMargins();
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

			const HWND hWnd = this->Handle();

			NCCALCSIZE_PARAMS* params = (NCCALCSIZE_PARAMS*)lParam;
			RECT& clientRect = params->rgrc[0];

			// 保存原始上边框位置
			const LONG originalTop = clientRect.top;

			// 应用默认边框
			LRESULT ret = DefWindowProc(hWnd, WM_NCCALCSIZE, wParam, lParam);
			if (ret != 0) {
				return ret;
			}

			// 重新应用原始上边框，因此我们完全移除了默认边框中的上边框和标题栏，但保留了其他方向的边框
			clientRect.top = originalTop;

			// WM_NCCALCSIZE 在 WM_SIZE 前
			_isMaximized = IsMaximized(hWnd);
			
			if (_isMaximized) {
				// 最大化的窗口的实际尺寸比屏幕的工作区更大一点，这是为了将可调整窗口大小的区域隐藏在屏幕外面
				clientRect.top += _GetResizeHandleHeight();

				// 如果有自动隐藏的任务栏，我们在它的方向稍微减小客户区，这样用户就可以用鼠标呼出任务栏
				if (HMONITOR hMon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST)) {
					MONITORINFO monInfo{ .cbSize = sizeof(MONITORINFO) };
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

			// 如果在 WM_SIZE 中处理会导致窗口闪烁
			_UpdateFrameMargins();

			return 0;
		}
		case WM_PAINT:
		{
			if (Win32Helper::GetOSVersion().IsWin11()) {
				break;
			}

			PAINTSTRUCT ps{ 0 };
			HDC hdc = BeginPaint(this->Handle(), &ps);
			if (!hdc) {
				return 0;
			}

			const int topBorderHeight = (int)_GetTopBorderThickness();

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

				static bool isLightBrush = _isLightTheme;
				static HBRUSH backgroundBrush = CreateSolidBrush(isLightBrush ?
					ThemeHelper::LIGHT_TINT_COLOR : ThemeHelper::DARK_TINT_COLOR);

				if (isLightBrush != _isLightTheme) {
					isLightBrush = _isLightTheme;
					DeleteBrush(backgroundBrush);
					backgroundBrush = CreateSolidBrush(isLightBrush ?
						ThemeHelper::LIGHT_TINT_COLOR : ThemeHelper::DARK_TINT_COLOR);
				}

				if (!isLightBrush) {
					// 这里我们想要黑色背景而不是原始边框
					// hack 来自 https://github.com/microsoft/terminal/blob/0ee2c74cd432eda153f3f3e77588164cde95044f/src/cascadia/WindowsTerminal/NonClientIslandWindow.cpp#L1030-L1047
					HDC opaqueDc;
					BP_PAINTPARAMS params = {
						.cbSize = sizeof(params),
						.dwFlags = BPPF_NOCLIP | BPPF_ERASE
					};
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

			EndPaint(this->Handle(), &ps);
			return 0;
		}
		case WM_KEYDOWN:
		{
			if (wParam == VK_TAB) {
				// 处理焦点
				if (_xamlSource) {
					using namespace winrt::Windows::UI::Xaml::Hosting;

					XamlSourceFocusNavigationReason reason = (GetKeyState(VK_SHIFT) & 0x80) ?
						XamlSourceFocusNavigationReason::Last : XamlSourceFocusNavigationReason::First;
					_xamlSource.NavigateFocus(XamlSourceFocusNavigationRequest(reason));
				}
				return 0;
			}
			break;
		}
		case WM_DPICHANGED:
		{
			_UpdateDpi(HIWORD(wParam));

			RECT* newRect = (RECT*)lParam;
			SetWindowPos(this->Handle(),
				NULL,
				newRect->left,
				newRect->top,
				newRect->right - newRect->left,
				newRect->bottom - newRect->top,
				SWP_NOZORDER | SWP_NOACTIVATE
			);

			DpiChanged.Invoke(_currentDpi);

			return 0;
		}
		case WM_MOVING:
		{
			if (_hwndXamlIsland) {
				XamlHelper::RepositionXamlPopups(_content->XamlRoot(), false);
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
			// 根据文档，wParam 的低四位供系统内部使用
			switch (wParam & 0xFFF0) {
			case SC_MINIMIZE:
			{
				// 最小化前关闭 ComboBox。不能在 WM_SIZE 中处理，该消息发送于最小化之后，会导致 ComboBox 无法交互
				if (_content) {
					XamlHelper::CloseComboBoxPopup(_content->XamlRoot());
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
		case WM_ACTIVATE:
		{
			if (LOWORD(wParam) == WA_INACTIVE && _content) {
				XamlHelper::CloseComboBoxPopup(_content->XamlRoot());
			}

			return 0;
		}
		case WM_SIZE:
		{
			_isMaximized = IsMaximized(this->Handle());

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

					_content->Dispatcher().RunAsync(winrt::CoreDispatcherPriority::Normal, [xamlRoot(_content->XamlRoot())]() {
						XamlHelper::RepositionXamlPopups(xamlRoot, true);
					});
				}
			}

			return 0;
		}
		case WM_DESTROY:
		{
			// 确保关闭过程中 _content 已经为空
			_content = nullptr;

			_xamlSourceNative2 = nullptr;
			// 必须手动重置 Content，否则会内存泄露，使 RootPage 无法析构
			_xamlSource.Content(nullptr);
			_xamlSource.Close();
			_xamlSource = nullptr;
			_hwndXamlIsland = NULL;

			_isMaximized = false;
			_isLightTheme = true;

			Destroyed.Invoke();

			LRESULT ret = base_type::_MessageHandler(msg, wParam, lParam);

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

			return ret;
		}
		}

		return base_type::_MessageHandler(msg, wParam, lParam);
	}

	uint32_t _GetTopBorderThickness() const noexcept {
		// 最大化时没有上边框
		return _isMaximized ? 0 : _nativeTopBorderThickness;
	}

	int _GetResizeHandleHeight() const noexcept {
		// 没有 SM_CYPADDEDBORDER
		return GetSystemMetricsForDpi(SM_CXPADDEDBORDER, _currentDpi) +
			GetSystemMetricsForDpi(SM_CYSIZEFRAME, _currentDpi);
	}

	// 以最大化显示时使 XAML Islands 窗口尺寸保持稳定，见 _UpdateIslandPosition
	void _SetInitialMaximized(bool value) noexcept {
		_isInitialMaximized = value;
	}

	bool _IsInitialMaximized() const noexcept {
		return _isInitialMaximized;
	}

private:
	void _UpdateIslandPosition(int width, int height) const noexcept {
		// 当窗口以最大化显示时，为了修正动画，实际上是先窗口化显示然后改为最大化。这个过程中此函数
		// 会被调用多次，而且传入的尺寸不一致。我们想让 XAML Islands 窗口尺寸保持稳定以避免不必要
		// 的动画，因此需要估算最大化窗口的尺寸。
		// 由于没考虑自动隐藏的任务栏，计算结果可能不准确，但最多相差两个像素，可以接受。
		if (_isInitialMaximized && !_isMaximized) {
			if (HMONITOR hMon = MonitorFromWindow(this->Handle(), MONITOR_DEFAULTTONEAREST)) {
				MONITORINFO monInfo{};
				monInfo.cbSize = sizeof(MONITORINFO);
				GetMonitorInfo(hMon, &monInfo);

				// 最大化窗口的尺寸为当前屏幕工作区的尺寸
				width = monInfo.rcWork.right - monInfo.rcMonitor.left;
				height = monInfo.rcWork.bottom - monInfo.rcMonitor.top;
			}
		}

		// Win10 中上边框被涂黑来显示系统原始边框，Win11 中 DWM 绘制的上边框也位于客户区内，
		// 很可能是为了和 Win10 兼容。XAML Islands 不应该和上边框重叠。
		const int topBorderHeight = (int)_GetTopBorderThickness();

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

	void _UpdateFrameMargins() const noexcept {
		if (Win32Helper::GetOSVersion().IsWin11()) {
			return;
		}

		MARGINS margins{};
		if (_GetTopBorderThickness() > 0) {
			// 在 Win10 中，移除标题栏时上边框也被没了。我们的解决方案是: 使用 DwmExtendFrameIntoClientArea
			// 将边框扩展到客户区，然后在顶部绘制了一个黑色实线来显示系统原始边框（这种情况下操作系统将黑色视
			// 为透明）。因此我们有**完美**的上边框！
			// 见 https://docs.microsoft.com/en-us/windows/win32/dwm/customframe#extending-the-client-frame
			// 
			// 有的软件自己绘制了假的上边框，如 Chromium 系、WinUI 3 等，但窗口失去焦点时边框是半透明的，无法
			// 完美模拟。
			//
			// 我们选择扩展到标题栏高度，这是最好的选择。一个自然的想法是，既然上边框只有一个像素高，我们扩展一
			// 个像素即可，可惜因为 DWM 的 bug，这会使窗口失去焦点时上边框变为透明。那么能否传一个负值，让边框
			// 扩展到整个客户区？这大部分情况下可以工作，有一个小 bug: 不显示边框颜色的设置下深色模式的边框会变
			// 为纯黑而不是半透明。
			RECT frame{};
			AdjustWindowRectExForDpi(&frame, GetWindowStyle(this->Handle()), FALSE, 0, _currentDpi);
			margins.cyTopHeight = -frame.top;
		}
		DwmExtendFrameIntoClientArea(this->Handle(), &margins);
	}

	void _UpdateDpi(uint32_t dpi) noexcept {
		_currentDpi = dpi;

		// Win10 中窗口边框始终只有一个像素宽，Win11 中的窗口边框宽度和 DPI 缩放有关
		if (Win32Helper::GetOSVersion().IsWin11()) {
			HRESULT hr = DwmGetWindowAttribute(
				this->Handle(),
				DWMWA_VISIBLE_FRAME_BORDER_THICKNESS,
				&_nativeTopBorderThickness,
				sizeof(_nativeTopBorderThickness)
			);
			if (FAILED(hr)) {
				Logger::Get().ComError("DwmGetWindowAttribute 失败", hr);
			}
		}
	}

	HWND _hwndXamlIsland = NULL;
	winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource _xamlSource{ nullptr };
	winrt::com_ptr<IDesktopWindowXamlSourceNative2> _xamlSourceNative2;

	C _content{ nullptr };

	uint32_t _currentDpi = USER_DEFAULT_SCREEN_DPI;
	uint32_t _nativeTopBorderThickness = 1;

	bool _isLightTheme = true;
	bool _isInitialMaximized = false;
	bool _isMaximized = false;
};

}
