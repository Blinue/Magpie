#include "pch.h"
#include "PrintWindowFrameSource.h"
#include "App.h"


extern std::shared_ptr<spdlog::logger> logger;

bool PrintWindowFrameSource::Update() {
	HDC hdcDest;
	HRESULT hr = _dxgiSurface->GetDC(TRUE, &hdcDest);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("从 Texture2D 获取 IDXGISurface1 失败", hr));
		return false;
	}

	// https://chromium.googlesource.com/chromium/src.git/+/refs/heads/main/ui/snapshot/snapshot_win.cc
	// 不支持子窗口
	if (!PrintWindow(App::GetInstance().GetHwndSrc(), hdcDest, PW_CLIENTONLY | PW_RENDERFULLCONTENT)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("PrintWindow 失败"));
	}

	_dxgiSurface->ReleaseDC(nullptr);

	return true;
}
