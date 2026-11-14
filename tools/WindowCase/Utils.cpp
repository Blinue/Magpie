#include "pch.h"
#include "Utils.h"

RECT Utils::MonitorRectFromWindow(HWND hWnd) noexcept {
	HMONITOR hMon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi{ sizeof(mi) };
	GetMonitorInfo(hMon, &mi);
	return mi.rcMonitor;
}
