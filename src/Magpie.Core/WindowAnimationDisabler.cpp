#include "pch.h"
#include "WindowAnimationDisabler.h"
#include "Logger.h"
#include <dwmapi.h>

WindowAnimationDisabler::WindowAnimationDisabler(HWND hWnd) : _hWnd(hWnd) {
	BOOL value = TRUE;
	HRESULT hr = DwmSetWindowAttribute(hWnd, DWMWA_TRANSITIONS_FORCEDISABLED, &value, sizeof(value));
	if (FAILED(hr)) {
		Logger::Get().ComError("DwmSetWindowAttribute 失败", hr);
	}
}

WindowAnimationDisabler::~WindowAnimationDisabler() {
	if (!IsWindow(_hWnd)) {
		return;
	}

	BOOL value = FALSE;
	HRESULT hr = DwmSetWindowAttribute(_hWnd, DWMWA_TRANSITIONS_FORCEDISABLED, &value, sizeof(value));
	if (FAILED(hr)) {
		Logger::Get().ComError("DwmSetWindowAttribute 失败", hr);
	}
}
