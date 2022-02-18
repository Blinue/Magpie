#include "pch.h"
#include "FrameSourceBase.h"
#include "Utils.h"
#include "App.h"
#include "Logger.h"


FrameSourceBase::~FrameSourceBase() {
	HWND hwndSrc = App::Get().GetHwndSrc();

	// 还原窗口圆角
	if (_roundCornerDisabled) {
		_roundCornerDisabled = false;

		INT attr = DWMWCP_DEFAULT;
		HRESULT hr = DwmSetWindowAttribute(hwndSrc, DWMWA_WINDOW_CORNER_PREFERENCE, &attr, sizeof(attr));
		if (FAILED(hr)) {
			Logger::Get().ComError("取消禁用窗口圆角失败", hr);
		} else {
			Logger::Get().Info("已取消禁用窗口圆角");
		}
	}

	// 还原窗口大小调整
	if (_windowResizingDisabled) {
		_windowResizingDisabled = false;

		LONG_PTR style = GetWindowLongPtr(hwndSrc, GWL_STYLE);
		if (!(style & WS_THICKFRAME)) {
			if (SetWindowLongPtr(hwndSrc, GWL_STYLE, style | WS_THICKFRAME)) {
				if (!SetWindowPos(hwndSrc, 0, 0, 0, 0, 0,
					SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED)) {
					Logger::Get().Win32Error("SetWindowPos 失败");
				}

				Logger::Get().Info("已取消禁用窗口大小调整");
			} else {
				Logger::Get().Win32Error("取消禁用窗口大小调整失败");
			}
		}
	}
}

bool FrameSourceBase::Initialize() {
	HWND hwndSrc = App::Get().GetHwndSrc();

	// 禁用窗口大小调整
	if (App::Get().IsDisableWindowResizing()) {
		LONG_PTR style = GetWindowLongPtr(hwndSrc, GWL_STYLE);
		if (style & WS_THICKFRAME) {
			if (SetWindowLongPtr(hwndSrc, GWL_STYLE, style ^ WS_THICKFRAME)) {
				// 不重绘边框，以防某些窗口状态不正确
				// if (!SetWindowPos(hwndSrc, 0, 0, 0, 0, 0,
				//	SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED)) {
				//	SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("SetWindowPos 失败"));
				// }

				Logger::Get().Info("已禁用窗口大小调整");
				_windowResizingDisabled = true;
			} else {
				Logger::Get().Win32Error("禁用窗口大小调整失败");
			}
		}
	}

	// 禁用窗口圆角
	if (_HasRoundCornerInWin11()) {
		const auto& version = Utils::GetOSVersion();
		bool isWin11 = Utils::CompareVersion(
			version.dwMajorVersion, version.dwMinorVersion,
			version.dwBuildNumber, 10, 0, 22000) >= 0;

		if (isWin11) {
			INT attr = DWMWCP_DONOTROUND;
			HRESULT hr = DwmSetWindowAttribute(hwndSrc, DWMWA_WINDOW_CORNER_PREFERENCE, &attr, sizeof(attr));
			if (FAILED(hr)) {
				Logger::Get().ComError("禁用窗口圆角失败", hr);
			} else {
				Logger::Get().Info("已禁用窗口圆角");
				_roundCornerDisabled = true;
			}
		}
	}

	return true;
}

bool FrameSourceBase::_GetMapToOriginDPI(HWND hWnd, double& a, double& bx, double& by) {
	// HDC 中的 HBITMAP 尺寸为窗口的原始尺寸
	// 通过 GetWindowRect 获得的尺寸为窗口的 DPI 缩放后尺寸
	// 它们的商即为窗口的 DPI 缩放
	HDC hdcWindow = GetDCEx(hWnd, NULL, DCX_LOCKWINDOWUPDATE | DCX_WINDOW);
	if (!hdcWindow) {
		Logger::Get().Win32Error("GetDCEx 失败");
		return false;
	}

	HDC hdcClient = GetDCEx(hWnd, NULL, DCX_LOCKWINDOWUPDATE);
	if (!hdcClient) {
		Logger::Get().Win32Error("GetDCEx 失败");
		ReleaseDC(hWnd, hdcWindow);
		return false;
	}

	Utils::ScopeExit se([hWnd, hdcWindow, hdcClient]() {
		ReleaseDC(hWnd, hdcWindow);
		ReleaseDC(hWnd, hdcClient);
	});

	HGDIOBJ hBmpWindow = GetCurrentObject(hdcWindow, OBJ_BITMAP);
	if (!hBmpWindow) {
		Logger::Get().Win32Error("GetCurrentObject 失败");
		return false;
	}

	if (GetObjectType(hBmpWindow) != OBJ_BITMAP) {
		Logger::Get().Error("无法获取窗口的重定向表面");
		return false;
	}

	BITMAP bmp{};
	if (!GetObject(hBmpWindow, sizeof(bmp), &bmp)) {
		Logger::Get().Win32Error("GetObject 失败");
		return false;
	}

	RECT rect;
	if (!GetWindowRect(hWnd, &rect)) {
		Logger::Get().Win32Error("GetWindowRect 失败");
		return false;
	}

	a = bmp.bmWidth / double(rect.right - rect.left);
	
	// 使用 DPI 缩放无法可靠计算出窗口客户区的位置
	// 这里使用窗口 HDC 和客户区 HDC 的原点坐标差值
	// GetDCOrgEx 获得的是 DC 原点的屏幕坐标

	POINT ptClient{}, ptWindow{};
	if (!GetDCOrgEx(hdcClient, &ptClient)) {
		Logger::Get().Win32Error("GetDCOrgEx 失败");
		return false;
	}
	if (!GetDCOrgEx(hdcWindow, &ptWindow)) {
		Logger::Get().Win32Error("GetDCOrgEx 失败");
		return false;
	}

	if (!Utils::GetClientScreenRect(hWnd, rect)) {
		Logger::Get().Error("GetClientScreenRect 失败");
		return false;
	}

	// 以窗口的客户区左上角为基准
	// 该点在坐标系 1 中坐标为 (rect.left, rect.top)
	// 在坐标系 2 中坐标为 (ptClient.x - ptWindow.x, ptClient.y - ptWindow.y)
	// 由此计算出 b
	bx = ptClient.x - ptWindow.x - rect.left * a;
	by = ptClient.y - ptWindow.y - rect.top * a;
	
	return true;
}

bool FrameSourceBase::_CenterWindowIfNecessary(HWND hWnd, const RECT& rcWork) {
	RECT srcRect;
	if (!GetWindowRect(hWnd, &srcRect)) {
		Logger::Get().Win32Error("GetWindowRect 失败");
		return false;
	}

	if (srcRect.left < rcWork.left || srcRect.top < rcWork.top
		|| srcRect.right > rcWork.right || srcRect.bottom > rcWork.bottom) {
		// 源窗口超越边界，将源窗口移到屏幕中央
		SIZE srcSize = { srcRect.right - srcRect.left, srcRect.bottom - srcRect.top };
		SIZE rcWorkSize = { rcWork.right - rcWork.left, rcWork.bottom - rcWork.top };
		if (srcSize.cx > rcWorkSize.cx || srcSize.cy > rcWorkSize.cy) {
			// 源窗口无法被当前屏幕容纳，因此无法捕获
			App::Get().SetErrorMsg(ErrorMessages::SRC_TOO_LARGE);
			return false;
		}

		if (!SetWindowPos(
			hWnd,
			0,
			rcWork.left + (rcWorkSize.cx - srcSize.cx) / 2,
			rcWork.top + (rcWorkSize.cy - srcSize.cy) / 2,
			0,
			0,
			SWP_NOSIZE | SWP_NOZORDER
		)) {
			Logger::Get().Win32Error("SetWindowPos 失败");
		}
	}

	return true;
}

struct EnumChildWndParam {
	const wchar_t* clientWndClassName = nullptr;
	std::vector<HWND> childWindows;
};

static BOOL CALLBACK EnumChildProc(
	_In_ HWND   hwnd,
	_In_ LPARAM lParam
) {
	std::wstring className(256, 0);
	int num = GetClassName(hwnd, &className[0], (int)className.size());
	if (num == 0) {
		Logger::Get().Win32Error("GetClassName 失败");
		return TRUE;
	}
	className.resize(num);

	EnumChildWndParam* param = (EnumChildWndParam*)lParam;
	if (className == param->clientWndClassName) {
		param->childWindows.push_back(hwnd);
	}

	return TRUE;
}

static HWND FindClientWindow(HWND hwndSrc, const wchar_t* clientWndClassName) {
	// 查找所有窗口类名为 ApplicationFrameInputSinkWindow 的子窗口
	// 该子窗口一般为客户区
	EnumChildWndParam param{};
	param.clientWndClassName = clientWndClassName;
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

bool FrameSourceBase::_UpdateSrcFrameRect() {
	_srcFrameRect = {};

	HWND hwndSrc = App::Get().GetHwndSrc();

	if (App::Get().IsCropTitleBarOfUWP()) {
		std::wstring className(256, 0);
		int num = GetClassName(hwndSrc, &className[0], (int)className.size());
		if (num > 0) {
			className.resize(num);
			if (App::Get().IsCropTitleBarOfUWP() &&
				(className == L"ApplicationFrameWindow" || className == L"Windows.UI.Core.CoreWindow")
				) {
				// "Modern App"
				// 客户区窗口类名为 ApplicationFrameInputSinkWindow
				HWND hwndClient = FindClientWindow(hwndSrc, L"ApplicationFrameInputSinkWindow");
				if (hwndClient) {
					if (!Utils::GetClientScreenRect(hwndClient, _srcFrameRect)) {
						Logger::Get().Win32Error("GetClientScreenRect 失败");
					}
				}
			}
		} else {
			Logger::Get().Win32Error("GetClassName 失败");
		}
	}

	if (_srcFrameRect == RECT{}) {
		if (!Utils::GetClientScreenRect(hwndSrc, _srcFrameRect)) {
			Logger::Get().Win32Error("GetClientScreenRect 失败");
			return false;
		}
	}

	const RECT& cropBorders = App::Get().GetCropBorders();
	_srcFrameRect = {
		_srcFrameRect.left + cropBorders.left,
		_srcFrameRect.top + cropBorders.top,
		_srcFrameRect.right - cropBorders.right,
		_srcFrameRect.bottom - cropBorders.bottom
	};

	if (_srcFrameRect.right - _srcFrameRect.left <= 0 || _srcFrameRect.bottom - _srcFrameRect.top <= 0) {
		App::Get().SetErrorMsg(ErrorMessages::FAILED_TO_CROP);
		Logger::Get().Error("裁剪窗口失败");
		return false;
	}

	return true;
}
