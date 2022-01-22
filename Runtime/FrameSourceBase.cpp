#include "pch.h"
#include "FrameSourceBase.h"
#include "Utils.h"
#include "App.h"


extern std::shared_ptr<spdlog::logger> logger;


bool FrameSourceBase::_GetMapToOriginDPI(HWND hWnd, double& a, double& bx, double& by) {
	// HDC 中的 HBITMAP 尺寸为窗口的原始尺寸
	// 通过 GetWindowRect 获得的尺寸为窗口的 DPI 缩放后尺寸
	// 它们的商即为窗口的 DPI 缩放
	HDC hdcWindow = GetDCEx(hWnd, NULL, DCX_LOCKWINDOWUPDATE | DCX_WINDOW);
	if (!hdcWindow) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetDCEx 失败"));
		return false;
	}

	HDC hdcClient = GetDCEx(hWnd, NULL, DCX_LOCKWINDOWUPDATE);
	if (!hdcClient) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetDCEx 失败"));
		ReleaseDC(hWnd, hdcWindow);
		return false;
	}

	Utils::ScopeExit se([hWnd, hdcWindow, hdcClient]() {
		ReleaseDC(hWnd, hdcWindow);
		ReleaseDC(hWnd, hdcClient);
	});

	HGDIOBJ hBmpWindow = GetCurrentObject(hdcWindow, OBJ_BITMAP);
	if (!hBmpWindow) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetCurrentObject 失败"));
		return false;
	}

	if (GetObjectType(hBmpWindow) != OBJ_BITMAP) {
		SPDLOG_LOGGER_ERROR(logger, "无法获取窗口的重定向表面");
		return false;
	}

	BITMAP bmp{};
	if (!GetObject(hBmpWindow, sizeof(bmp), &bmp)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetObject 失败"));
		return false;
	}

	RECT rect;
	if (!GetWindowRect(hWnd, &rect)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetWindowRect 失败"));
		return false;
	}

	a = bmp.bmWidth / double(rect.right - rect.left);
	
	// 使用 DPI 缩放无法可靠计算出窗口客户区的位置
	// 这里使用窗口 HDC 和客户区 HDC 的原点坐标差值
	// GetDCOrgEx 获得的是 DC 原点的屏幕坐标

	POINT ptClient{}, ptWindow{};
	if (!GetDCOrgEx(hdcClient, &ptClient)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetDCOrgEx 失败"));
		return false;
	}
	if (!GetDCOrgEx(hdcWindow, &ptWindow)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetDCOrgEx 失败"));
		return false;
	}

	if (!Utils::GetClientScreenRect(hWnd, rect)) {
		SPDLOG_LOGGER_ERROR(logger, "GetClientScreenRect 失败");
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
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetWindowRect 失败"));
		return false;
	}

	if (srcRect.left < rcWork.left || srcRect.top < rcWork.top
		|| srcRect.right > rcWork.right || srcRect.bottom > rcWork.bottom) {
		// 源窗口超越边界，将源窗口移到屏幕中央
		SIZE srcSize = { srcRect.right - srcRect.left, srcRect.bottom - srcRect.top };
		SIZE rcWorkSize = { rcWork.right - rcWork.left, rcWork.bottom - rcWork.top };
		if (srcSize.cx > rcWorkSize.cx || srcSize.cy > rcWorkSize.cy) {
			// 源窗口无法被当前屏幕容纳，因此无法捕获
			App::GetInstance().SetErrorMsg(ErrorMessages::SRC_TOO_LARGE);
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
			SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("SetWindowPos 失败"));
		}
	}

	return true;
}
