#include "pch.h"
#include "SrcInfo.h"
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

bool SrcInfo::Set(HWND hWnd, const ScalingOptions& options) noexcept {
	_hWnd = hWnd;

	if (!IsWindow(_hWnd)) {
		Logger::Get().Error("源窗口句柄非法");
		return false;
	}

	if (!IsWindowVisible(_hWnd)) {
		Logger::Get().Error("不支持缩放隐藏的窗口");
		return false;
	}

	const HMONITOR hMon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONULL);
	if (!hMon) {
		Logger::Get().Error("源窗口不在任何屏幕上");
		return false;
	}

	_isFocused = GetForegroundWindow() == hWnd;

	if (!GetWindowRect(hWnd, &_windowRect)) {
		Logger::Get().Win32Error("GetWindowRect 失败");
		return false;
	}

	RECT clientRect;
	if (!Win32Helper::GetClientScreenRect(hWnd, clientRect)) {
		Logger::Get().Win32Error("GetClientScreenRect 失败");
		return false;
	}

	const UINT showCmd = Win32Helper::GetWindowShowCmd(hWnd);
	if (showCmd == SW_SHOWMINIMIZED) {
		Logger::Get().Error("不支持缩放最小化的窗口");
		return false;
	}

	_isMaximized = showCmd == SW_SHOWMAXIMIZED;

	// 计算窗口样式
	BOOL hasBorder = TRUE;
	HRESULT hr = DwmGetWindowAttribute(hWnd, DWMWA_NCRENDERING_ENABLED, &hasBorder, sizeof(hasBorder));
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

	return _CalcFrameRect(options);
}

bool SrcInfo::UpdateState(HWND hwndFore) noexcept {
	if (!IsWindow(_hWnd)) {
		Logger::Get().Error("源窗口已销毁");
		return false;
	}

	if (!IsWindowVisible(_hWnd)) {
		Logger::Get().Error("源窗口已隐藏");
		return false;
	}

	if (!GetWindowRect(_hWnd, &_windowRect)) {
		Logger::Get().Win32Error("GetWindowRect 失败");
		return false;
	}

	UINT showCmd = Win32Helper::GetWindowShowCmd(_hWnd);
	if (showCmd == SW_SHOWMINIMIZED) {
		Logger::Get().Error("源窗口处于最小化状态");
		return false;
	}
	_isMaximized = showCmd == SW_SHOWMAXIMIZED;

	_isFocused = hwndFore == _hWnd;
	return true;
}

bool SrcInfo::_CalcFrameRect(const ScalingOptions& options) noexcept {
	if (_windowKind == SrcWindowKind::NoDecoration) {
		// NoDecoration 类型的窗口不裁剪非客户区。它们要么没有非客户区，要么非客户区不是由
		// DWM 绘制，前者无需裁剪，后者不能裁剪。
		_frameRect = _windowRect;
	} else {
		// UWP 窗口都是 NoTitleBar 类型，但可能使用子窗口作为“客户区”
		if (_windowKind != SrcWindowKind::NoTitleBar || !GetClientRectOfUWP(_hWnd, _frameRect)) {
			if (!Win32Helper::GetClientScreenRect(_hWnd, _frameRect)) {
				Logger::Get().Error("GetClientScreenRect 失败");
				return false;
			}
		}

		if (_windowKind == SrcWindowKind::NoBorder && Win32Helper::GetOSVersion().IsWin11()) {
			// NoBorder 类型的窗口在 Win11 中边框被绘制到客户区内
			_frameRect.left += _topBorderThicknessInClient;
			_frameRect.right -= _topBorderThicknessInClient;
			_frameRect.bottom -= _topBorderThicknessInClient;
		}

		// 左右下三边始终裁剪至客户区，上边框需特殊处理。OnlyThickFrame 类型的窗口有着很宽
		// 的上边框，某种意义上也是标题栏，但捕获它没有意义。
		if (options.IsCaptureTitleBar() && _windowKind != SrcWindowKind::OnlyThickFrame) {
			// 捕获标题栏时将标题栏视为“客户区”，需裁剪原生标题栏的上边框
			_frameRect.top = _windowRect.top + _topBorderThicknessInClient;
		} else  if (_windowRect.top == _frameRect.top) {
			// 如果上边框在客户区内，则裁剪上边框
			_frameRect.top += _topBorderThicknessInClient;
		}
	}

	if (_isMaximized) {
		// 最大化的窗口只有屏幕内是有效区域
		HMONITOR hMon = MonitorFromWindow(_hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO mi{ .cbSize = sizeof(mi) };
		if (!GetMonitorInfo(hMon, &mi)) {
			Logger::Get().Win32Error("GetMonitorInfo 失败");
			return false;
		}

		IntersectRect(&_frameRect, &_frameRect, &mi.rcMonitor);
	}

	_frameRect = {
		std::lround(_frameRect.left + options.cropping.Left),
		std::lround(_frameRect.top + options.cropping.Top),
		std::lround(_frameRect.right - options.cropping.Right),
		std::lround(_frameRect.bottom - options.cropping.Bottom)
	};

	if (_frameRect.right - _frameRect.left <= 0 || _frameRect.bottom - _frameRect.top <= 0) {
		Logger::Get().Error("裁剪窗口失败");
		return false;
	}

	return true;
}

}
