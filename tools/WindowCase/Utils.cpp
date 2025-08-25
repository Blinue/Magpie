#include "pch.h"
#include "Utils.h"

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

RECT Utils::MonitorRectFromWindow(HWND hWnd) noexcept {
	HMONITOR hMon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi{ sizeof(mi) };
	GetMonitorInfo(hMon, &mi);
	return mi.rcMonitor;
}

// https://github.com/microsoft/wil/blob/1fae8ac13f393d727d6c3dba50b1dfe3f63e835b/include/wil/win32_helpers.h#L788
HINSTANCE Utils::GetModuleInstanceHandle() noexcept {
	return reinterpret_cast<HINSTANCE>(&__ImageBase);
}
