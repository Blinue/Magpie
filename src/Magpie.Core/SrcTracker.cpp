#include "pch.h"
#include "SrcTracker.h"
#include "Logger.h"
#include "SmallVector.h"
#include "Win32Helper.h"
#ifdef _DEBUG
#include "WindowHelper.h"
#endif
#include <dwmapi.h>
#include <ShellScalingApi.h>

namespace Magpie {

static bool GetWindowIntegrityLevel(HWND hWnd, DWORD& integrityLevel) noexcept {
	wil::unique_process_handle hProc = Win32Helper::GetWindowProcessHandle(hWnd);
	if (!hProc) {
		Logger::Get().Error("GetWindowProcessHandle 失败");
		return false;
	}

	wil::unique_handle hQueryToken;
	if (!OpenProcessToken(hProc.get(), TOKEN_QUERY, hQueryToken.put())) {
		Logger::Get().Win32Error("OpenProcessToken 失败");
		return false;
	}

	return Win32Helper::GetProcessIntegrityLevel(hQueryToken.get(), integrityLevel);
}

static bool CheckIL(HWND hwndSrc) noexcept {
	static DWORD thisIL = []() -> DWORD {
		DWORD il;
		return Win32Helper::GetProcessIntegrityLevel(NULL, il) ? il : 0;
	}();

	DWORD windowIL;
	return GetWindowIntegrityLevel(hwndSrc, windowIL) && windowIL <= thisIL;
}

static bool IsWindowMoving(HWND hWnd) noexcept {
	GUITHREADINFO guiThreadInfo{ .cbSize = sizeof(GUITHREADINFO) };
	if (GetGUIThreadInfo(GetWindowThreadProcessId(hWnd, nullptr), &guiThreadInfo)) {
		return guiThreadInfo.flags & GUI_INMOVESIZE;
	} else {
		Logger::Get().Win32Error("GetGUIThreadInfo 失败");
		return false;
	}
}

ScalingError SrcTracker::Set(HWND hWnd, const ScalingOptions& options, bool& isInvisibleOrMinimized) noexcept {
	assert(!isInvisibleOrMinimized);

	_hWnd = hWnd;
	
	// 这里不检查源窗口是否挂起，将在创建缩放窗口前检查

	if (!IsWindow(_hWnd)) {
		Logger::Get().Error("源窗口句柄非法");
		return ScalingError::InvalidSourceWindow;
	}

	// 不可见和最小化的窗口将等待源窗口状态改变，这里提前返回。注意 showCmd 不能准确
	// 判断窗口可见性，应使用 IsWindowVisible。
	if (!IsWindowVisible(_hWnd)) {
		isInvisibleOrMinimized = true;
		return ScalingError::NoError;
	}

	const UINT showCmd = Win32Helper::GetWindowShowCmd(hWnd);
	if (showCmd == SW_SHOWMINIMIZED) {
		isInvisibleOrMinimized = true;
		return ScalingError::NoError;
	}

	_isMaximized = showCmd == SW_SHOWMAXIMIZED;

	if (Win32Helper::GetWindowClassName(hWnd) == L"Ghost") {
		Logger::Get().Error("不支持缩放幽灵窗口");
		return ScalingError::InvalidSourceWindow;
	}

	if (!CheckIL(hWnd)) {
		Logger::Get().Error("不支持缩放 IL 更高的窗口");
		return ScalingError::LowIntegrityLevel;
	}

	// 已在 ScalingService 中阻止
	assert(!WindowHelper::IsForbiddenSystemWindow(hWnd));

	if (GetWindowLongPtr(hWnd, GWL_EXSTYLE) & WS_EX_TRANSPARENT) {
		Logger::Get().Error("不支持缩放透明的窗口");
		return ScalingError::InvalidSourceWindow;
	}

	const HMONITOR hMon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONULL);
	if (!hMon) {
		Logger::Get().Error("源窗口不在任何屏幕上");
		return ScalingError::InvalidSourceWindow;
	}

	_isFocused = GetForegroundWindow() == hWnd;
	_isMoving = IsWindowMoving(_hWnd);

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

	// 计算窗口样式
	BOOL hasBorder = TRUE;
	bool hasCustomNonclient = false;
	hr = DwmGetWindowAttribute(hWnd, DWMWA_NCRENDERING_ENABLED, &hasBorder, sizeof(hasBorder));
	if (FAILED(hr) || !hasBorder) {
		// 无原生框架要么是因为无 WS_CAPTION 和 WS_THICKFRAME 样式，要么禁用了原生窗口框架
		// 以自绘标题栏和边框。
		_windowKind = SrcWindowKind::NoNativeFrame;
		hasCustomNonclient = GetWindowStyle(hWnd) & (WS_CAPTION | WS_THICKFRAME);
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
				// 还原阴影，这种窗口实际上是 NoNativeFrame 类型。遗憾的是没有办法获知窗口是否调用了
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

	// * 最大化窗口、NoNativeFrame 窗口和 Win10 中 NoBorder 窗口不存在边框
	LONG borderThicknessInFrame = 0;
	const bool isWin11 = Win32Helper::GetOSVersion().IsWin11();
	if (!_isMaximized &&
		_windowKind != SrcWindowKind::NoNativeFrame &&
		(_windowKind != SrcWindowKind::NoBorder || isWin11))
	{
		// 使用屏幕而非窗口的 DPI 来计算边框宽度
		UINT dpi = USER_DEFAULT_SCREEN_DPI;
		GetDpiForMonitor(hMon, MDT_EFFECTIVE_DPI, &dpi, &dpi);

		borderThicknessInFrame = (LONG)Win32Helper::GetNativeWindowBorderThickness(dpi);
	}

	return _CalcSrcRect(options, hasCustomNonclient, borderThicknessInFrame);
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
	bool& isInvisibleOrMinimized,
	bool& focusedChanged,
	bool& rectChanged,
	bool& sizeChanged,
	bool& movingChanged
) noexcept {
	assert(!isInvisibleOrMinimized && !focusedChanged &&
		!rectChanged && !sizeChanged && !movingChanged);

	if (!IsWindow(_hWnd)) {
		Logger::Get().Info("源窗口已销毁");
		return false;
	}

	isInvisibleOrMinimized = !IsWindowVisible(_hWnd);

	// 缩放中途对源窗口响应性的检查更宽松，即使源窗口挂起一段时间，只要用户不做额
	// 外的操作就不会终止缩放，直到源窗口被替换为幽灵窗口。
	// 
	// 不要使用 IsHungAppWindow，它有误报的情况，见 GH#1244。这里用了未记录函数
	// GhostWindowFromHungWindow，它可以准确检查源窗口是否已被替换为幽灵窗口。
	static const auto ghostWindowFromHungWindow =
		Win32Helper::LoadSystemFunction<HWND WINAPI(HWND)>(
			L"user32.dll", "GhostWindowFromHungWindow");
	if (ghostWindowFromHungWindow && ghostWindowFromHungWindow(_hWnd)) {
		Logger::Get().Info("源窗口已挂起");
		return false;
	}

	if (_isFocused != (hwndFore == _hWnd)) {
		_isFocused = !_isFocused;
		focusedChanged = true;
	}

	const bool oldMaximized = _isMaximized;

	WINDOWPLACEMENT wp{ sizeof(wp) };
	if (!GetWindowPlacement(_hWnd, &wp)) {
		Logger::Get().Win32Error("GetWindowPlacement 失败");
		return false;
	}

	_isMaximized = wp.showCmd == SW_SHOWMAXIMIZED;

	RECT curWindowRect;
	if (wp.showCmd == SW_SHOWMINIMIZED) {
		isInvisibleOrMinimized = true;

		// rcNormalPosition 使用工作区坐标，应转换为屏幕坐标
		HMONITOR hMon = MonitorFromWindow(_hWnd, MONITOR_DEFAULTTOPRIMARY);
		MONITORINFO mi{ sizeof(mi) };
		if (!GetMonitorInfo(hMon, &mi)) {
			Logger::Get().Win32Error("GetMonitorInfo 失败");
			return false;
		}

		curWindowRect = wp.rcNormalPosition;
		Win32Helper::OffsetRect(
			curWindowRect, mi.rcWork.left - mi.rcMonitor.left, mi.rcWork.top - mi.rcMonitor.top);
	} else {
		if (!GetWindowRect(_hWnd, &curWindowRect)) {
			Logger::Get().Win32Error("GetWindowRect 失败");
			return false;
		}
	}

	sizeChanged = oldMaximized != _isMaximized ||
		Win32Helper::GetSizeOfRect(curWindowRect) != Win32Helper::GetSizeOfRect(_windowRect);
	if (sizeChanged) {
		rectChanged = true;
		return true;
	}

	// 缩放窗口正在调整大小或被拖动时源窗口的移动是异步的，暂时不检查源窗口是否移动
	if (isResizingOrMoving) {
		rectChanged = oldMaximized != _isMaximized;
		return true;
	}

	// 最大化状态改变视为尺寸发生变化
	rectChanged = oldMaximized != _isMaximized || curWindowRect != _windowRect;
	
	if (isWindowedMode && !sizeChanged) {
		if (rectChanged) {
			const LONG offsetX = curWindowRect.left - _windowRect.left;
			const LONG offsetY = curWindowRect.top - _windowRect.top;
			Win32Helper::OffsetRect(_windowFrameRect, offsetX, offsetY);
			Win32Helper::OffsetRect(_srcRect, offsetX, offsetY);
		}

		// 处理自己实现拖拽逻辑的窗口：将鼠标左键按下视为开始拖拽，释放视为拖拽结束。
		// 可能会有误判，但幸好后果不太严重。
		const bool isMoving = !isInvisibleOrMinimized &&
			(IsWindowMoving(_hWnd) || (rectChanged && IsPrimaryMouseButtonDown()));
		if (_isMoving != isMoving) {
			movingChanged = true;
			_isMoving = isMoving;
		}
	}
	
	if (rectChanged) {
		_windowRect = curWindowRect;
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

	if (Win32Helper::IsWindowHung(_hWnd)) {
		Logger::Get().Error("源窗口已挂起");
		return false;
	}

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

static HWND FindClientWindowOfUWP(HWND hwndSrc, const wchar_t* clientWndClassName) noexcept {
	// 查找所有窗口类名为 ApplicationFrameInputSinkWindow 的子窗口，
	// 该窗口一般为客户区。
	struct EnumData {
		const wchar_t* clientWndClassName;
		SmallVector<HWND, 1> childWindows;
	} data{ clientWndClassName };

	static const auto enumChildProc = [](HWND hWnd, LPARAM lParam) {
		std::wstring className = Win32Helper::GetWindowClassName(hWnd);

		EnumData& data = *(EnumData*)lParam;
		if (className == data.clientWndClassName) {
			data.childWindows.push_back(hWnd);
		}

		return TRUE;
	};

	EnumChildWindows(hwndSrc, enumChildProc, (LPARAM)&data);

	if (data.childWindows.empty()) {
		// 未找到符合条件的子窗口
		return hwndSrc;
	}

	if (data.childWindows.size() == 1) {
		return data.childWindows[0];
	}

	// 如果有多个匹配的子窗口，取最大的（一般不会出现）
	int maxSize = 0;
	uint32_t maxIdx = 0;
	for (uint32_t i = 0, end = (uint32_t)data.childWindows.size(); i < end; ++i) {
		RECT rect;
		if (!GetClientRect(data.childWindows[i], &rect)) {
			continue;
		}

		int size = rect.right - rect.left + rect.bottom - rect.top;
		if (size > maxSize) {
			maxSize = size;
			maxIdx = i;
		}
	}

	return data.childWindows[maxIdx];
}

static bool GetClientRectOfUWP(HWND hWnd, RECT& rect) noexcept {
	std::wstring className = Win32Helper::GetWindowClassName(hWnd);
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

bool SrcTracker::SetFocus() const noexcept {
	// 不要把被禁用的窗口设为前台，检查是否存在弹窗
	if (IsWindowEnabled(_hWnd)) {
		return SetForegroundWindow(_hWnd);
	}

	const HWND hwndPopup = GetWindow(_hWnd, GW_ENABLEDPOPUP);
	if (IsWindowEnabled(hwndPopup)) {
		return SetForegroundWindow(hwndPopup);
	}

	// 源窗口被禁用视为成功
	return true;
}

ScalingError SrcTracker::_CalcSrcRect(
	const ScalingOptions& options,
	bool hasCustomNonclient,
	LONG borderThicknessInFrame
) noexcept {
	if (_windowKind == SrcWindowKind::NoNativeFrame) {
		if (hasCustomNonclient) {
			if (options.RealIsCaptureTitleBar()) {
				// 窗口的非客户区是自绘的，无法模拟，因此启用捕获标题栏时捕获整个窗口
				_srcRect = _windowRect;
			} else {
				RECT clientRect;
				if (!Win32Helper::GetClientScreenRect(_hWnd, clientRect)) {
					Logger::Get().Error("GetClientScreenRect 失败");
					return ScalingError::ScalingFailedGeneral;
				}

				// 如果有滚动条需特殊处理
				if (const DWORD srcStyle = GetWindowStyle(_hWnd); srcStyle & (WS_VSCROLL | WS_HSCROLL)) {
					// 左右两边不可能都有滚动条，据此找到边框宽度
					const LONG borderThickness = std::min(
						clientRect.left - _windowRect.left,
						_windowRect.right - clientRect.right
					);

					if (srcStyle & WS_VSCROLL) {
						_srcRect.left = _windowRect.left + borderThickness;
						_srcRect.right = _windowRect.right - borderThickness;
					} else {
						_srcRect.left = clientRect.left;
						_srcRect.right = clientRect.right;
					}

					if (srcStyle & WS_HSCROLL) {
						_srcRect.bottom = _windowRect.bottom - borderThickness;
					} else {
						_srcRect.bottom = clientRect.bottom;
					}

					_srcRect.top = clientRect.top;
				} else {
					_srcRect = clientRect;
				}
			}
		} else {
			// 不存在非客户区
			_srcRect = _windowRect;
		}
	} else {
		const bool isCaptureTitleBar = options.RealIsCaptureTitleBar();
		
		// UWP 窗口都是 NoTitleBar 类型，但可能使用子窗口作为“客户区”
		if (_windowKind == SrcWindowKind::NoTitleBar && !isCaptureTitleBar && GetClientRectOfUWP(_hWnd, _srcRect)) {
			_srcRect.top = std::max(_srcRect.top, _windowFrameRect.top + borderThicknessInFrame);
		} else {
			// 不要使用客户区矩形，它不包含滚动条
			_srcRect.left = _windowFrameRect.left + borderThicknessInFrame;
			_srcRect.top = _windowFrameRect.top + borderThicknessInFrame;
			_srcRect.right = _windowFrameRect.right - borderThicknessInFrame;
			_srcRect.bottom = _windowFrameRect.bottom - borderThicknessInFrame;

			if (!isCaptureTitleBar || _windowKind == SrcWindowKind::OnlyThickFrame) {
				RECT clientRect;
				if (!Win32Helper::GetClientScreenRect(_hWnd, clientRect)) {
					Logger::Get().Error("GetClientScreenRect 失败");
					return ScalingError::ScalingFailedGeneral;
				}

				_srcRect.top = std::max(_srcRect.top, clientRect.top);
			}
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
