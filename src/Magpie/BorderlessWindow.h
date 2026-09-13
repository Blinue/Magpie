#pragma once
#include "BaseWindow.h"
#include "Win32Helper.h"
#include <dwmapi.h>
#include <shellapi.h>

namespace Magpie {

template <typename T>
class BorderlessWindow : public BaseWindow<T> {
	using base_type = BaseWindow<T>;
	friend base_type;

public:
	// 使得调用基类方法时不用限定
	using base_type::Handle;

	uint32_t GetDpi() const noexcept {
		return _dpi;
	}

protected:
	bool _IsMaximized() const noexcept {
		return _isMaximized;
	}

	uint32_t _GetTopBorderThickness() const noexcept {
		// 最大化时没有上边框
		return _isMaximized ? 0 : _nativeBorderThickness;
	}

	// 派生类可重载这个函数来跳过绘制背景。Win10 中应总是绘制背景
	bool _ShouldDrawBackground() const noexcept {
		return true;
	}

	// 派生类可重载这个函数来绘制背景
	void _DrawBackground(HDC /*hdc*/, const RECT& /*bkgRect*/) const noexcept {}

	LRESULT _MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
		switch (msg) {
		case WM_CREATE:
		{
			_UpdateDpi(GetDpiForWindow(Handle()));
			_UpdateFrameMargins();
			return 0;
		}
		case WM_DPICHANGED:
		{
			_UpdateDpi(HIWORD(wParam));

			RECT* newRect = (RECT*)lParam;
			SetWindowPos(
				Handle(),
				NULL,
				newRect->left,
				newRect->top,
				newRect->right - newRect->left,
				newRect->bottom - newRect->top,
				SWP_NOZORDER | SWP_NOACTIVATE
			);

			return 0;
		}
		case WM_SIZE:
		{
			_isMaximized = IsMaximized(Handle());
			return 0;
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
			LRESULT ret = DefWindowProc(Handle(), WM_NCCALCSIZE, wParam, lParam);
			if (ret != 0) {
				return ret;
			}

			// 重新应用原始上边框，因此我们完全移除了默认边框中的上边框和标题栏，但保留了其他方向的边框
			clientRect.top = originalTop;

			// WM_NCCALCSIZE 在 WM_SIZE 前
			_isMaximized = IsMaximized(Handle());

			if (_isMaximized) {
				// 最大化的窗口的实际尺寸比屏幕的工作区更大一点，这是为了将可调整窗口大小的区域隐藏在屏幕外面
				clientRect.top += _GetResizeHandleHeight();

				// 如果有自动隐藏的任务栏，我们在它的方向稍微减小客户区，这样用户就可以用鼠标呼出任务栏
				if (HMONITOR hMon = MonitorFromWindow(Handle(), MONITOR_DEFAULTTONEAREST)) {
					MONITORINFO monInfo = { .cbSize = sizeof(MONITORINFO) };
					GetMonitorInfo(hMon, &monInfo);

					// 检查是否有自动隐藏的任务栏
					APPBARDATA appBarData = { .cbSize = sizeof(appBarData) };
					if (SHAppBarMessage(ABM_GETSTATE, &appBarData) & ABS_AUTOHIDE) {
						// 检查显示器的一条边
						auto hasAutohideTaskbar = [&monInfo](UINT edge) -> bool {
							APPBARDATA data = {
								.cbSize = sizeof(data),
								.uEdge = edge,
								.rc = monInfo.rcMonitor
							};
							HWND hTaskbar = (HWND)SHAppBarMessage(ABM_GETAUTOHIDEBAREX, &data);
							return hTaskbar != nullptr;
						};

						constexpr int AUTO_HIDE_TASKBAR_HEIGHT = 2;

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
		case WM_NCHITTEST:
		{
			const POINT cursorPos = { GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam) };

			RECT clientRect;
			Win32Helper::GetClientScreenRect(Handle(), clientRect);

			// 如果光标不在客户区内则交给 OS 处理
			if (!PtInRect(&clientRect, cursorPos)) {
				break;
			}

			// 处理上边缘
			if (!_isMaximized) {
				const int resizeHandleHeight = _GetResizeHandleHeight();
				if (cursorPos.y < clientRect.top + resizeHandleHeight) {
					if (cursorPos.x < clientRect.left + resizeHandleHeight) {
						return HTTOPLEFT;
					} else if (cursorPos.x >= clientRect.right - resizeHandleHeight) {
						return HTTOPRIGHT;
					} else {
						return HTTOP;
					}
				}
			}

			// 派生类可进一步细分出标题栏区域
			return HTCLIENT;
		}
		case WM_PAINT:
		{
			if (!((T*)this)->_ShouldDrawBackground()) {
				return 0;
			}

			PAINTSTRUCT ps{};
			HDC hdc = BeginPaint(Handle(), &ps);
			if (!hdc) {
				return 0;
			}

			int topBorderThickness = (int)_GetTopBorderThickness();

			// Win10 中在顶部绘制黑色实线以显示系统原始边框，见 _UpdateFrameMargins
			if (Win32Helper::GetOSVersion().IsWin10() && ps.rcPaint.top < topBorderThickness) {
				RECT topBorderRect = {
					ps.rcPaint.left,
					ps.rcPaint.top,
					ps.rcPaint.right,
					topBorderThickness
				};

				static HBRUSH hBrush = GetStockBrush(BLACK_BRUSH);
				FillRect(hdc, &topBorderRect, hBrush);
			}

			if (ps.rcPaint.bottom > topBorderThickness) {
				((T*)this)->_DrawBackground(hdc,
					RECT{ ps.rcPaint.left, topBorderThickness, ps.rcPaint.right, ps.rcPaint.bottom });
			}

			EndPaint(Handle(), &ps);
			return 0;
		}
		case WM_MENUCHAR:
		{
			// 防止按 Alt+Key 时发出铃声
			return MAKELRESULT(0, MNC_CLOSE);
		}
		}

		return base_type::_MessageHandler(msg, wParam, lParam);
	}

private:
	void _UpdateDpi(uint32_t dpi) noexcept {
		_dpi = dpi;

		// Win10 中窗口边框始终只有一个像素宽，Win11 中的窗口边框宽度和 DPI 缩放有关
		if (Win32Helper::GetOSVersion().IsWin11()) {
			HRESULT hr = DwmGetWindowAttribute(
				this->Handle(),
				DWMWA_VISIBLE_FRAME_BORDER_THICKNESS,
				&_nativeBorderThickness,
				sizeof(_nativeBorderThickness)
			);
			if (FAILED(hr)) {
				Logger::Get().ComError("DwmGetWindowAttribute 失败", hr);
			}
		}
	}

	void _UpdateFrameMargins() const noexcept {
		if (Win32Helper::GetOSVersion().IsWin11()) {
			return;
		}

		// Win10 中应总是绘制背景
		assert(((T*)this)->_ShouldDrawBackground());

		MARGINS margins{};
		if (_GetTopBorderThickness() > 0) {
			// 在 Win10 中，移除标题栏时上边框也被没了。我们的解决方案是：使用 DwmExtendFrameIntoClientArea
			// 将边框扩展到客户区，然后在顶部绘制了一个黑色实线来显示系统原始边框（这种情况下操作系统将黑色视
			// 为透明）。因此我们有**完美**的上边框！
			// 见 https://docs.microsoft.com/en-us/windows/win32/dwm/customframe#extending-the-client-frame
			// 
			// 有的软件自己绘制了假的上边框，如 Chromium 系、WinUI 3 等，但窗口失去焦点时边框是半透明的，无法
			// 完美模拟。
			//
			// 扩展到标题栏高度是最好的选择。一个自然的想法是，既然上边框只有一个像素高，我们扩展一个像素即可，
			// 可惜因为 DWM 的 bug，这会使窗口失去焦点时上边框变为透明。那么能否传一个负值，让边框扩展到整个客
			// 户区？这大部分情况下可以工作，有一个小 bug：不显示边框颜色的设置下深色模式的边框会变为纯黑而不
			// 是半透明。
			RECT frame{};
			AdjustWindowRectExForDpi(&frame, GetWindowStyle(Handle()), FALSE, 0, _dpi);
			margins.cyTopHeight = -frame.top;
		}
		DwmExtendFrameIntoClientArea(Handle(), &margins);
	}

	int _GetResizeHandleHeight() const noexcept {
		// 没有 SM_CYPADDEDBORDER
		return GetSystemMetricsForDpi(SM_CXPADDEDBORDER, _dpi) +
			GetSystemMetricsForDpi(SM_CYSIZEFRAME, _dpi);
	}

	uint32_t _dpi = USER_DEFAULT_SCREEN_DPI;
	uint32_t _nativeBorderThickness = 1;
	bool _isMaximized = false;
};

}
