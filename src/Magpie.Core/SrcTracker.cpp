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

ScalingError SrcTracker::Set(HWND hWnd, const ScalingOptions& options) noexcept {
	_hWnd = hWnd;
	_isMoving = false;

	// 这里不检查源窗口是否挂起，将在创建缩放窗口前检查

	if (!IsWindow(_hWnd)) {
		Logger::Get().Error("源窗口句柄非法");
		return ScalingError::InvalidSourceWindow;
	}

	if (!IsWindowVisible(_hWnd)) {
		Logger::Get().Error("不支持缩放隐藏的窗口");
		return ScalingError::InvalidSourceWindow;
	}

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

	const HWND hwndFore = GetForegroundWindow();
	_isFocused = hwndFore == hWnd;
	_UpdateIsOwnedWindowFocused(hwndFore);

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
		return ScalingError::InvalidSourceWindow;
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

	// * 最大化窗口、NoDecoration 窗口和 Win10 中 NoBorder 窗口不存在边框
	LONG borderThicknessInFrame = 0;
	const bool isWin11 = Win32Helper::GetOSVersion().IsWin11();
	if (!_isMaximized &&
		_windowKind != SrcWindowKind::NoDecoration &&
		(_windowKind != SrcWindowKind::NoBorder || isWin11))
	{
		// 使用屏幕而非窗口的 DPI 来计算边框宽度
		UINT dpi = USER_DEFAULT_SCREEN_DPI;
		GetDpiForMonitor(hMon, MDT_EFFECTIVE_DPI, &dpi, &dpi);

		borderThicknessInFrame = (LONG)Win32Helper::GetNativeWindowBorderThickness(dpi);
	}

	return _CalcSrcRect(options, borderThicknessInFrame);
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
	bool& focusedChanged,
	bool& ownedWindowFocusedChanged,
	bool& rectChanged,
	bool& sizeChanged,
	bool& movingChanged
) noexcept {
	assert(!focusedChanged && !ownedWindowFocusedChanged && !rectChanged && !sizeChanged && !movingChanged);

	if (!IsWindow(_hWnd)) {
		Logger::Get().Error("源窗口已销毁");
		return false;
	}

	if (!IsWindowVisible(_hWnd)) {
		Logger::Get().Error("源窗口已隐藏");
		return false;
	}

	// Win32Helper::IsWindowHung 更准确，但它会向源窗口发送消息，比较耗时。
	// IsHungAppWindow 的另一个好处是它不如 Win32Helper::IsWindowHung 严
	// 格，因此即使源窗口挂起一段时间，只要用户不做额外的操作就不会结束缩放，
	// 直到源窗口被替换为幽灵窗口。
	if (IsHungAppWindow(_hWnd)) {
		Logger::Get().Error("源窗口已挂起");
		return false;
	}

	if (_isFocused != (hwndFore == _hWnd)) {
		_isFocused = !_isFocused;
		focusedChanged = true;
	}

	ownedWindowFocusedChanged = _UpdateIsOwnedWindowFocused(hwndFore);

	const bool oldMaximized = _isMaximized;
	UINT showCmd = Win32Helper::GetWindowShowCmd(_hWnd);
	if (showCmd == SW_SHOWMINIMIZED) {
		Logger::Get().Error("源窗口处于最小化状态");
		return false;
	}
	_isMaximized = showCmd == SW_SHOWMAXIMIZED;

	RECT curWindowRect;
	if (!GetWindowRect(_hWnd, &curWindowRect)) {
		Logger::Get().Win32Error("GetWindowRect 失败");
		return false;
	}

	sizeChanged = oldMaximized != _isMaximized ||
		Win32Helper::GetSizeOfRect(curWindowRect) != Win32Helper::GetSizeOfRect(_windowRect);

	// 缩放窗口正在调整大小或被拖动时源窗口的移动是异步的，暂时不检查源窗口是否移动
	if (isResizingOrMoving) {
		rectChanged = oldMaximized != _isMaximized;
		return true;
	}

	// 最大化状态改变视为尺寸发生变化
	rectChanged = oldMaximized != _isMaximized || curWindowRect != _windowRect;
	
	if (isWindowedMode && !sizeChanged) {
		bool isMoving = false;
		GUITHREADINFO guiThreadInfo{ .cbSize = sizeof(GUITHREADINFO) };
		if (GetGUIThreadInfo(GetWindowThreadProcessId(_hWnd, nullptr), &guiThreadInfo)) {
			isMoving = guiThreadInfo.flags & GUI_INMOVESIZE;
		} else {
			Logger::Get().Win32Error("GetGUIThreadInfo 失败");
		}

		// 处理自己实现拖拽逻辑的窗口：将鼠标左键按下视为开始拖拽，释放视为拖拽结束。
		// 可能会有误判，但幸好后果不太严重。
		if (_isMoving || (!_isMoving && rectChanged)) {
			isMoving = isMoving || IsPrimaryMouseButtonDown();
		}

		if (rectChanged) {
			const LONG offsetX = curWindowRect.left - _windowRect.left;
			const LONG offsetY = curWindowRect.top - _windowRect.top;
			Win32Helper::OffsetRect(_windowFrameRect, offsetX, offsetY);
			Win32Helper::OffsetRect(_srcRect, offsetX, offsetY);
		}

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
	if (_isOwnedWindowFocused) {
		return true;
	}

	// 如果源窗口存在弹窗（即被源窗口所有的窗口），应把弹窗设为前台窗口
	const HWND hwndPopup = GetWindow(_hWnd, GW_ENABLEDPOPUP);
	return SetForegroundWindow(hwndPopup ? hwndPopup : _hWnd);
}

ScalingError SrcTracker::_CalcSrcRect(const ScalingOptions& options, LONG borderThicknessInFrame) noexcept {
	if (_windowKind == SrcWindowKind::NoDecoration) {
		// NoDecoration 类型的窗口不裁剪非客户区。它们要么没有非客户区，要么非客户区不是由
		// DWM 绘制，前者无需裁剪，后者不能裁剪。
		_srcRect = _windowRect;
	} else {
		// UWP 窗口都是 NoTitleBar 类型，但可能使用子窗口作为“客户区”
		if (_windowKind == SrcWindowKind::NoTitleBar && !options.IsCaptureTitleBar() && GetClientRectOfUWP(_hWnd, _srcRect)) {
			_srcRect.top = std::max(_srcRect.top, _windowFrameRect.top + borderThicknessInFrame);
		} else {
			_srcRect.left = _windowFrameRect.left + borderThicknessInFrame;
			_srcRect.top = _windowFrameRect.top + borderThicknessInFrame;
			_srcRect.right = _windowFrameRect.right - borderThicknessInFrame;
			_srcRect.bottom = _windowFrameRect.bottom - borderThicknessInFrame;

			if (!options.IsCaptureTitleBar() || _windowKind == SrcWindowKind::OnlyThickFrame) {
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

static bool IsOwnedWindow(HWND hwndOwner, HWND hwndTest) noexcept {
	HWND hwndCur = hwndTest;
	while (bool(hwndCur = GetWindowOwner(hwndCur))) {
		if (hwndCur == hwndOwner) {
			return true;
		}
	}
	return false;
}

bool SrcTracker::_UpdateIsOwnedWindowFocused(HWND hwndFore) noexcept {
	// 支持两种形式的弹窗
	// 1. 弹窗被源窗口所有
	// 2. 弹窗没有被源窗口所有，但弹出时源窗口被禁用
	bool newValue = !_isFocused && (IsOwnedWindow(_hWnd, hwndFore) || !IsWindowEnabled(_hWnd));
	if (_isOwnedWindowFocused == newValue) {
		return false;
	} else {
		_isOwnedWindowFocused = newValue;
		return true;
	}
}

}
