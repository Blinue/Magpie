#include "pch.h"
#include "FrameSourceBase.h"
#include "Utils.h"


extern std::shared_ptr<spdlog::logger> logger;


bool FrameSourceBase::_GetWindowDpiScale(HWND hWnd, float& dpiScale) {
	// HDC 中的 HBITMAP 尺寸为窗口的原始尺寸
	// 通过 GetWindowRect 获得的尺寸为窗口的 DPI 缩放后尺寸
	// 它们的商即为窗口的 DPI 缩放
	HDC hdcWindow = GetDCEx(hWnd, NULL, DCX_LOCKWINDOWUPDATE | DCX_WINDOW);
	if (!hdcWindow) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetDCEx 失败"));
		return false;
	}
	
	Utils::ScopeExit se([hWnd, hdcWindow]() {
		ReleaseDC(hWnd, hdcWindow);
	});

	HBITMAP hBmpWindow = (HBITMAP)GetCurrentObject(hdcWindow, OBJ_BITMAP);
	if (!hBmpWindow) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetCurrentObject 失败"));
		return false;
	}

	BITMAP bmp{};
	if (!GetObject(hBmpWindow, sizeof(bmp), &bmp)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetObject 失败"));
		return false;
	}

	RECT rectWindow;
	if (!GetWindowRect(hWnd, &rectWindow)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetWindowRect 失败"));
		return false;
	}

	dpiScale = float(rectWindow.right - rectWindow.left) / bmp.bmWidth;
	return true;
}

bool FrameSourceBase::_GetDpiAwareWindowClientOffset(HWND hWnd, POINT& clientOffset) {
	// 使用 DPI 缩放也无法可靠计算出窗口客户区的位置
	// 这里使用窗口 HDC 和客户区 HDC 的原点坐标差值
	// GetDCOrgEx 获得的是 DC 原点的屏幕坐标

	HDC hdcClient = GetDCEx(hWnd, NULL, DCX_LOCKWINDOWUPDATE);
	if (!hdcClient) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetDCEx 失败"));
		return false;
	}

	HDC hdcWindow = GetDCEx(hWnd, NULL, DCX_LOCKWINDOWUPDATE | DCX_WINDOW);
	if (!hdcWindow) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetDCEx 失败"));
		ReleaseDC(hWnd, hdcClient);
		return false;
	}

	Utils::ScopeExit se([hWnd, hdcClient, hdcWindow]() {
		ReleaseDC(hWnd, hdcClient);
		ReleaseDC(hWnd, hdcWindow);
	});

	POINT ptClient{}, ptWindow{};
	if (!GetDCOrgEx(hdcClient, &ptClient)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetDCOrgEx 失败"));
		return false;
	}
	if (!GetDCOrgEx(hdcWindow, &ptWindow)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetDCOrgEx 失败"));
		return false;
	}

	clientOffset = { ptClient.x - ptWindow.x, ptClient.y - ptWindow.y };
	return true;
}
