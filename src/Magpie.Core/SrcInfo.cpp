#include "pch.h"
#include "SrcInfo.h"
#include "Win32Helper.h"
#include "Logger.h"
#include <dwmapi.h>
#include "SmallVector.h"

namespace Magpie {

// 获取窗口边框宽度
static uint32_t CalcWindowBorderThickness(HWND hWnd, const RECT& wndRect) noexcept {
	if (Win32Helper::GetWindowShowCmd(hWnd) != SW_SHOWNORMAL) {
		// 最大化的窗口不存在边框
		return 0;
	}

	// 检查该窗口是否禁用了非客户区域的绘制
	BOOL hasBorder = TRUE;
	HRESULT hr = DwmGetWindowAttribute(hWnd, DWMWA_NCRENDERING_ENABLED, &hasBorder, sizeof(hasBorder));
	if (FAILED(hr)) {
		Logger::Get().ComError("DwmGetWindowAttribute 失败", hr);
		return 0;
	}
	if (!hasBorder) {
		return 0;
	}

	RECT clientRect;
	if (!Win32Helper::GetClientScreenRect(hWnd, clientRect)) {
		Logger::Get().Win32Error("GetClientScreenRect 失败");
		return 0;
	}

	// 如果左右下三边均存在边框，那么应视为存在上边框:
	// * Win10 中窗口很可能绘制了假的上边框，这是很常见的创建无边框窗口的方法
	// * Win11 中 DWM 会将上边框绘制到客户区
	if (wndRect.top == clientRect.top && (wndRect.left == clientRect.left ||
		wndRect.right == clientRect.right || wndRect.bottom == clientRect.bottom)) {
		return 0;
	}

	if (Win32Helper::GetOSVersion().IsWin11()) {
		// Win11 的窗口边框宽度取决于 DPI
		uint32_t borderThickness = 0;
		hr = DwmGetWindowAttribute(hWnd, DWMWA_VISIBLE_FRAME_BORDER_THICKNESS, &borderThickness, sizeof(borderThickness));
		if (FAILED(hr)) {
			Logger::Get().ComError("DwmGetWindowAttribute 失败", hr);
			return 0;
		}

		return borderThickness;
	} else {
		// Win10 的窗口边框始终只有一个像素宽
		return 1;
	}
}

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

	if (!UpdateState(NULL)) {
		return false;
	}

	_borderThickness = CalcWindowBorderThickness(hWnd, _windowRect);

	return _CalcFrameRect(options);
}

bool SrcInfo::UpdateState(HWND hwndFore) noexcept {
	if (!GetWindowRect(_hWnd, &_windowRect)) {
		Logger::Get().Win32Error("GetWindowRect 失败");
		return false;
	}

	UINT showCmd = Win32Helper::GetWindowShowCmd(_hWnd);
	if (showCmd == SW_SHOWMINIMIZED) {
		Logger::Get().Win32Error("源窗口处于最小化状态");
		return false;
	}
	_isMaximized = showCmd == SW_SHOWMAXIMIZED;

	_isFocused = hwndFore == _hWnd;
	return true;
}

bool SrcInfo::_CalcFrameRect(const ScalingOptions& options) noexcept {
	if (options.IsCaptureTitleBar()) {
		if (!Win32Helper::GetWindowFrameRect(_hWnd, _frameRect)) {
			Logger::Get().Error("GetWindowFrameRect 失败");
			return false;
		}

		RECT clientRect;
		if (!Win32Helper::GetClientScreenRect(_hWnd, clientRect)) {
			Logger::Get().Win32Error("GetClientScreenRect 失败");
			return false;
		}

		// 左右下三边裁剪至客户区
		_frameRect.left = std::max(_frameRect.left, clientRect.left);
		_frameRect.right = std::min(_frameRect.right, clientRect.right);
		_frameRect.bottom = std::min(_frameRect.bottom, clientRect.bottom);

		// 裁剪上边框
		_frameRect.top += _borderThickness;
	} else {
		if (!GetClientRectOfUWP(_hWnd, _frameRect)) {
			if (!Win32Helper::GetClientScreenRect(_hWnd, _frameRect)) {
				Logger::Get().Error("GetClientScreenRect 失败");
				return false;
			}
		}

		if (Win32Helper::GetWindowShowCmd(_hWnd) == SW_SHOWMAXIMIZED) {
			// 最大化的窗口可能有一部分客户区在屏幕外，但只有屏幕内是有效区域，
			// 因此裁剪到屏幕边界
			HMONITOR hMon = MonitorFromWindow(_hWnd, MONITOR_DEFAULTTONEAREST);
			MONITORINFO mi{ .cbSize = sizeof(mi) };
			if (!GetMonitorInfo(hMon, &mi)) {
				Logger::Get().Win32Error("GetMonitorInfo 失败");
				return false;
			}

			IntersectRect(&_frameRect, &_frameRect, &mi.rcMonitor);
		} else {
			RECT windowRect;
			if (!GetWindowRect(_hWnd, &windowRect)) {
				Logger::Get().Win32Error("GetWindowRect 失败");
				return false;
			}

			// 如果上边框在客户区内，则裁剪上边框
			if (windowRect.top == _frameRect.top) {
				_frameRect.top += _borderThickness;
			}
		}
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
