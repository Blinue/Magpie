#include "pch.h"
#include "SrcTracker.h"
#include "Win32Helper.h"
#include "Logger.h"
#include <dwmapi.h>
#include "SmallVector.h"
#include <ShellScalingApi.h>

namespace Magpie {

struct EnumChildWndParam {
	const wchar_t* clientWndClassName = nullptr;
	SmallVector<HWND, 1> childWindows;
};

static BOOL CALLBACK EnumChildProc(
	_In_ HWND   hwnd,
	_In_ LPARAM lParam
) {
	std::wstring className = Win32Helper::GetWndClassName(hwnd);

	EnumChildWndParam* param = (EnumChildWndParam*)lParam;
	if (className == param->clientWndClassName) {
		param->childWindows.push_back(hwnd);
	}

	return TRUE;
}

static HWND FindClientWindowOfUWP(HWND hwndSrc, const wchar_t* clientWndClassName) noexcept {
	// 查找所有窗口类名为 ApplicationFrameInputSinkWindow 的子窗口
	// 该子窗口一般为客户区
	EnumChildWndParam param{ .clientWndClassName = clientWndClassName };
	EnumChildWindows(hwndSrc, EnumChildProc, (LPARAM)&param);

	if (param.childWindows.empty()) {
		// 未找到符合条件的子窗口
		return hwndSrc;
	}

	if (param.childWindows.size() == 1) {
		return param.childWindows[0];
	}

	// 如果有多个匹配的子窗口，取最大的（一般不会出现）
	int maxSize = 0, maxIdx = 0;
	for (int i = 0; i < param.childWindows.size(); ++i) {
		RECT rect;
		if (!GetClientRect(param.childWindows[i], &rect)) {
			continue;
		}

		int size = rect.right - rect.left + rect.bottom - rect.top;
		if (size > maxSize) {
			maxSize = size;
			maxIdx = i;
		}
	}

	return param.childWindows[maxIdx];
}

static bool GetClientRectOfUWP(HWND hWnd, RECT& rect) noexcept {
	std::wstring className = Win32Helper::GetWndClassName(hWnd);
	if (className != L"ApplicationFrameWindow" && className != L"Windows.UI.Core.CoreWindow") {
		return false;
	}

	// 客户区窗口类名为 ApplicationFrameInputSinkWindow
	HWND hwndClient = FindClientWindowOfUWP(hWnd, L"ApplicationFrameInputSinkWindow");
	if (!hwndClient) {
		return false;
	}

	if (!Win32Helper::GetClientScreenRect(hwndClient, rect)) {
		Logger::Get().Win32Error("GetClientScreenRect 失败");
		return false;
	}

	return true;
}

ScalingError SrcTracker::Set(HWND hWnd, const ScalingOptions& options) noexcept {
	_hWnd = hWnd;

	if (!IsWindow(_hWnd)) {
		Logger::Get().Error("源窗口句柄非法");
		return ScalingError::ScalingFailedGeneral;
	}

	if (!IsWindowVisible(_hWnd)) {
		Logger::Get().Error("不支持缩放隐藏的窗口");
		return ScalingError::ScalingFailedGeneral;
	}

	const HMONITOR hMon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONULL);
	if (!hMon) {
		Logger::Get().Error("源窗口不在任何屏幕上");
		return ScalingError::ScalingFailedGeneral;
	}

	_isFocused = GetForegroundWindow() == hWnd;

	if (!GetWindowRect(hWnd, &_windowRect)) {
		Logger::Get().Win32Error("GetWindowRect 失败");
		return ScalingError::ScalingFailedGeneral;
	}

	HRESULT hr = DwmGetWindowAttribute(hWnd, DWMWA_EXTENDED_FRAME_BOUNDS,
		&_windowFrameRect, sizeof(_windowFrameRect));
	if (FAILED(hr)) {
		Logger::Get().ComError("DwmGetWindowAttribute 失败", hr);
		return ScalingError::ScalingFailedGeneral;
	}

	RECT clientRect;
	if (!Win32Helper::GetClientScreenRect(hWnd, clientRect)) {
		Logger::Get().Win32Error("GetClientScreenRect 失败");
		return ScalingError::ScalingFailedGeneral;
	}

	const UINT showCmd = Win32Helper::GetWindowShowCmd(hWnd);
	if (showCmd == SW_SHOWMINIMIZED) {
		Logger::Get().Error("不支持缩放最小化的窗口");
		return ScalingError::ScalingFailedGeneral;
	}

	_isMaximized = showCmd == SW_SHOWMAXIMIZED;

	// 计算窗口样式
	BOOL hasBorder = TRUE;
	hr = DwmGetWindowAttribute(hWnd, DWMWA_NCRENDERING_ENABLED, &hasBorder, sizeof(hasBorder));
	if (FAILED(hr) || !hasBorder) {
		// 凡是没有原生框架的窗口都视为 NoDecoration，这类窗口可能没有 WS_CAPTION 和 WS_THICKFRAME 样式，
		// 或者禁用了原生窗口框架以自绘标题栏和边框。
		_windowKind = SrcWindowKind::NoDecoration;
	} else {
		// 最大化窗口的上边框很可能存在非客户区，这使得 NoTitleBar 类型的窗口最大化时会被归类到 Native。
		// 技术上说这很合理，上边框处的非客户区当然可以视为标题栏，对后续计算也没有影响。
		if (_windowRect.top == clientRect.top) {
			if (_windowRect.left != clientRect.left && _windowRect.right != clientRect.right && _windowRect.bottom != clientRect.bottom) {
				// 如果左右下三边均存在边框，那么应视为存在上边框:
				// * Win10 中窗口很可能绘制了假的上边框，这是很常见的创建无边框窗口的方法
				// * Win11 中 DWM 会将上边框绘制到客户区
				_windowKind = SrcWindowKind::NoTitleBar;
			} else {
				// 一个窗口要么有边框，要么没有。只要左右下三边中有一条边没有边框，我们就将它视为无边框窗口。
				// 
				// FIXME: 有的窗口（如微信）通过 WM_NCCALCSIZE 移除边框，但不使用 DwmExtendFrameIntoClientArea
				// 还原阴影，这种窗口实际上是 NoDecoration 类型。遗憾的是没有办法获知窗口是否调用了
				// DwmExtendFrameIntoClientArea，因此我们假设所有使用 WM_NCCALCSIZE 移除边框的窗口都有阴影，
				// 一方面有阴影的情况更多，比如基于 electron 的窗口，另一方面如果假设没有阴影会使得 Win11 中
				// 不能正确裁剪边框导致黑边，而如果假设有阴影，猜错的后果相对较轻。
				_windowKind = SrcWindowKind::NoBorder;
			}
		} else {
			const DWORD windowStyle = GetWindowStyle(hWnd);
			if (!(windowStyle & WS_CAPTION) && (windowStyle & WS_THICKFRAME)) {
				// 若无 WS_CAPTION 样式则系统边框仍存在但上边框较粗
				_windowKind = SrcWindowKind::OnlyThickFrame;
			} else {
				_windowKind = SrcWindowKind::Native;
			}
		}
	}

	// * 最大化的窗口不存在边框
	// * Win11 中“无边框”窗口存在边框
	// * 如果启用了捕获标题栏，则将标题栏视为“客户区”，此时 _topBorderThicknessInClient
	//   实际上是标题栏区域的上边框宽度
	const bool isWin11 = Win32Helper::GetOSVersion().IsWin11();
	if (_isMaximized
		|| (_windowKind == SrcWindowKind::NoBorder && !isWin11)
		|| _windowKind == SrcWindowKind::NoDecoration) {
		_topBorderThicknessInClient = 0;
	} else {
		// 使用屏幕而非窗口的 DPI 来计算边框宽度
		UINT dpi = USER_DEFAULT_SCREEN_DPI;
		GetDpiForMonitor(hMon, MDT_EFFECTIVE_DPI, &dpi, &dpi);

		_topBorderThicknessInClient = Win32Helper::GetNativeWindowBorderThickness(dpi);
	}

	return _CalcSrcRect(options);
}

static bool IsPrimaryMouseButtonDown() noexcept {
	const bool isSwapped = GetSystemMetrics(SM_SWAPBUTTON);
	const int vkPrimary = isSwapped ? VK_RBUTTON : VK_LBUTTON;
	return GetAsyncKeyState(vkPrimary) & 0x8000;
}

bool SrcTracker::UpdateState(
	HWND hwndFore,
	bool isWindowedMode,
	bool isResizingOrMoving,
	bool& srcRectChanged,
	bool& srcSizeChanged,
	bool& srcMovingChanged
) noexcept {
	assert(!srcRectChanged && !srcSizeChanged && !srcMovingChanged);

	if (!IsWindow(_hWnd)) {
		Logger::Get().Error("源窗口已销毁");
		return false;
	}

	if (!IsWindowVisible(_hWnd)) {
		Logger::Get().Error("源窗口已隐藏");
		return false;
	}

	_isFocused = hwndFore == _hWnd;

	const bool oldMaximized = _isMaximized;
	UINT showCmd = Win32Helper::GetWindowShowCmd(_hWnd);
	if (showCmd == SW_SHOWMINIMIZED) {
		Logger::Get().Error("源窗口处于最小化状态");
		return false;
	}
	_isMaximized = showCmd == SW_SHOWMAXIMIZED;

	// 缩放窗口正在调整大小或被拖动时源窗口的移动是异步的，暂时不检查源窗口矩形
	if (isResizingOrMoving) {
		srcSizeChanged = oldMaximized != _isMaximized;
		srcRectChanged = srcSizeChanged;
		return true;
	}

	const RECT oldWindowRect = _windowRect;
	if (!GetWindowRect(_hWnd, &_windowRect)) {
		Logger::Get().Win32Error("GetWindowRect 失败");
		return false;
	}

	// 最大化状态变化时视为尺寸发生变化
	srcRectChanged = oldMaximized != _isMaximized || oldWindowRect != _windowRect;
	srcSizeChanged = oldMaximized != _isMaximized ||
		Win32Helper::GetSizeOfRect(oldWindowRect) != Win32Helper::GetSizeOfRect(_windowRect);

	if (isWindowedMode && !srcSizeChanged) {
		bool isMoving = false;
		GUITHREADINFO guiThreadInfo{ .cbSize = sizeof(GUITHREADINFO) };
		if (GetGUIThreadInfo(GetWindowThreadProcessId(_hWnd, nullptr), &guiThreadInfo)) {
			isMoving = guiThreadInfo.flags & GUI_INMOVESIZE;
		} else {
			Logger::Get().Win32Error("GetGUIThreadInfo 失败");
		}

		// 处理自己实现拖拽逻辑的窗口：将鼠标左键按下视为开始拖拽，释放视为拖拽结束。
		// 可能会有误判，但幸好后果不太严重。
		if (_isMoving || (!_isMoving && srcRectChanged)) {
			isMoving = isMoving || IsPrimaryMouseButtonDown();
		}

		if (srcRectChanged) {
			const LONG offsetX = _windowRect.left - oldWindowRect.left;
			const LONG offsetY = _windowRect.top - oldWindowRect.top;
			Win32Helper::OffsetRect(_windowFrameRect, offsetX, offsetY);
			Win32Helper::OffsetRect(_srcRect, offsetX, offsetY);
		}

		if (_isMoving != isMoving) {
			srcMovingChanged = true;
			_isMoving = isMoving;
		}
	}
	
	return true;
}

bool SrcTracker::Move(int offsetX, int offsetY, bool async) noexcept {
	assert(!_isMaximized);

	if (offsetX == 0 && offsetY == 0) {
		return true;
	}

	if (!async) {
		// 同步移动
		Win32Helper::OffsetRect(_windowRect, offsetX, offsetY);
		Win32Helper::OffsetRect(_windowFrameRect, offsetX, offsetY);
		Win32Helper::OffsetRect(_srcRect, offsetX, offsetY);
		return MoveOnEndResizeMove();
	}

	// 异步移动源窗口
	if (!SetWindowPos(
		_hWnd,
		NULL,
		_windowRect.left + offsetX,
		_windowRect.top + offsetY,
		0,
		0,
		SWP_NOACTIVATE | SWP_NOREDRAW | SWP_NOSIZE | SWP_NOZORDER | SWP_ASYNCWINDOWPOS
	)) {
		Logger::Get().Win32Error("SetWindowPos 失败");
		return false;
	}

	// 暂时设为理想值，缩放窗口调整大小或移动结束后将在 MoveOnEndResizeMove
	// 更新为实际位置。
	Win32Helper::OffsetRect(_windowRect, offsetX, offsetY);
	Win32Helper::OffsetRect(_windowFrameRect, offsetX, offsetY);
	Win32Helper::OffsetRect(_srcRect, offsetX, offsetY);
	return true;
}

bool SrcTracker::MoveOnEndResizeMove() noexcept {
	assert(!_isMaximized);

	// 同步移动源窗口，这确保所有异步操作都已完成
	if (!SetWindowPos(
		_hWnd,
		NULL,
		_windowRect.left,
		_windowRect.top,
		0,
		0,
		SWP_NOACTIVATE | SWP_NOREDRAW | SWP_NOSIZE | SWP_NOZORDER
	)) {
		Logger::Get().Win32Error("SetWindowPos 失败");
		return false;
	}

	// 需要重新检索窗口矩形，因为 SetWindowPos 不保证准确设置。常见的情况是源窗口
	// 被 DPI 虚拟化时经常有轻微偏移，此外技术上说源窗口可以在 WM_WINDOWPOSCHANGING
	// 中随意改变尺寸和位置。
	const RECT oldWindowRect = _windowRect;

	if (!GetWindowRect(_hWnd, &_windowRect)) {
		Logger::Get().Win32Error("GetWindowRect 失败");
		return false;
	}

	if (Win32Helper::GetSizeOfRect(oldWindowRect) != Win32Helper::GetSizeOfRect(_windowRect)) {
		Logger::Get().Error("源窗口意外出现尺寸变化");
		return false;
	}

	const int offsetX = _windowRect.left - oldWindowRect.left;
	const int offsetY = _windowRect.top - oldWindowRect.top;
	if (offsetX != 0 || offsetY != 0) {
		Win32Helper::OffsetRect(_windowFrameRect, offsetX, offsetY);
		Win32Helper::OffsetRect(_srcRect, offsetX, offsetY);
	}
	
	return true;
}

ScalingError SrcTracker::_CalcSrcRect(const ScalingOptions& options) noexcept {
	if (_windowKind == SrcWindowKind::NoDecoration) {
		// NoDecoration 类型的窗口不裁剪非客户区。它们要么没有非客户区，要么非客户区不是由
		// DWM 绘制，前者无需裁剪，后者不能裁剪。
		_srcRect = _windowRect;
	} else {
		// UWP 窗口都是 NoTitleBar 类型，但可能使用子窗口作为“客户区”
		if (_windowKind != SrcWindowKind::NoTitleBar || !GetClientRectOfUWP(_hWnd, _srcRect)) {
			if (!Win32Helper::GetClientScreenRect(_hWnd, _srcRect)) {
				Logger::Get().Error("GetClientScreenRect 失败");
				return ScalingError::ScalingFailedGeneral;
			}
		}

		if (_windowKind == SrcWindowKind::NoBorder && Win32Helper::GetOSVersion().IsWin11()) {
			// NoBorder 类型的窗口在 Win11 中边框被绘制到客户区内
			_srcRect.left += _topBorderThicknessInClient;
			_srcRect.right -= _topBorderThicknessInClient;
			_srcRect.bottom -= _topBorderThicknessInClient;
		}

		// 左右下三边始终裁剪至客户区，上边框需特殊处理。OnlyThickFrame 类型的窗口有着很宽
		// 的上边框，某种意义上也是标题栏，但捕获它没有意义。
		if (options.IsCaptureTitleBar() && _windowKind != SrcWindowKind::OnlyThickFrame) {
			// 捕获标题栏时将标题栏视为“客户区”，需裁剪原生标题栏的上边框
			_srcRect.top = _windowRect.top + _topBorderThicknessInClient;
		} else  if (_windowRect.top == _srcRect.top) {
			// 如果上边框在客户区内，则裁剪上边框
			_srcRect.top += _topBorderThicknessInClient;
		}
	}

	if (_isMaximized) {
		// 最大化的窗口只有屏幕内是有效区域
		HMONITOR hMon = MonitorFromWindow(_hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO mi{ .cbSize = sizeof(mi) };
		if (!GetMonitorInfo(hMon, &mi)) {
			Logger::Get().Win32Error("GetMonitorInfo 失败");
			return ScalingError::ScalingFailedGeneral;
		}

		Win32Helper::IntersectRect(_srcRect, _srcRect, mi.rcMonitor);
	}

	static constexpr int MIN_SRC_SIZE = 64;

	if (_srcRect.right - _srcRect.left < MIN_SRC_SIZE || _srcRect.bottom - _srcRect.top < MIN_SRC_SIZE) {
		Logger::Get().Error("源窗口太小");
		return ScalingError::InvalidSourceWindow;
	}

	_srcRect = {
		std::lround(_srcRect.left + options.cropping.Left),
		std::lround(_srcRect.top + options.cropping.Top),
		std::lround(_srcRect.right - options.cropping.Right),
		std::lround(_srcRect.bottom - options.cropping.Bottom)
	};

	if (_srcRect.right - _srcRect.left < MIN_SRC_SIZE || _srcRect.bottom - _srcRect.top < MIN_SRC_SIZE) {
		Logger::Get().Error("裁剪窗口失败");
		return ScalingError::InvalidCropping;
	}

	return ScalingError::NoError;
}

}
