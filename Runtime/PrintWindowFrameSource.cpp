#include "pch.h"
#include "PrintWindowFrameSource.h"
#include "App.h"


extern std::shared_ptr<spdlog::logger> logger;

bool PrintWindowFrameSource::Initialize() {
	const RECT& srcClientRect = App::GetInstance().GetSrcClientRect();
	HWND hwndSrc = App::GetInstance().GetHwndSrc();
	HWND hwndSrcClient = App::GetInstance().GetHwndSrcClient();

	float dpiScale = -1;
	if (!_GetWindowDpiScale(hwndSrc, dpiScale)) {
		SPDLOG_LOGGER_ERROR(logger, "_GetWindowDpiScale 失败");
	}

	SPDLOG_LOGGER_INFO(logger, fmt::format("源窗口 DPI 缩放为 {}", dpiScale));

	bool needClip = hwndSrc != hwndSrcClient;

	RECT frameRect;
	if (needClip) {
		if (!GetClientRect(hwndSrc, &frameRect)) {
			SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetClientRect 失败"));
			return false;
		}
	}

	SIZE clientSize;
	SIZE frameSize;

	if (dpiScale > 0 && abs(dpiScale - 1.0f) > 1e-5f) {
		// DPI 感知
		clientSize = {
			(LONG)ceilf((srcClientRect.right - srcClientRect.left) / dpiScale),
			(LONG)ceilf((srcClientRect.bottom - srcClientRect.top) / dpiScale)
		};

		if (needClip) {
			frameSize = {
			(LONG)ceilf((frameRect.right - frameRect.left) / dpiScale),
			(LONG)ceilf((frameRect.bottom - frameRect.top) / dpiScale)
			};
		}
	} else {
		clientSize = { srcClientRect.right - srcClientRect.left, srcClientRect.bottom - srcClientRect.top };
		if (needClip) {
			frameSize = { frameRect.right - frameRect.left, frameRect.bottom - frameRect.top };
		}
	}

	if (needClip) {
		// clientSize 和 frameSize 尺寸不同则需要裁剪
		needClip &= clientSize.cx != frameSize.cx || clientSize.cy != frameSize.cy;
	}

	D3D11_TEXTURE2D_DESC desc{};
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.Width = clientSize.cx;
	desc.Height = clientSize.cy;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	desc.MiscFlags = needClip ? 0 : D3D11_RESOURCE_MISC_GDI_COMPATIBLE;
	HRESULT hr = App::GetInstance().GetRenderer().GetD3DDevice()->CreateTexture2D(&desc, nullptr, &_output);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("创建 Texture2D 失败", hr));
		return false;
	}

	hr = _output.As<IDXGISurface1>(&_outputSurface);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("从 Texture2D 获取 IDXGISurface1 失败", hr));
		return false;
	}

	if (needClip) {
		desc.Width = frameSize.cx;
		desc.Height = frameSize.cy;
		desc.MiscFlags = D3D11_RESOURCE_MISC_GDI_COMPATIBLE;
		hr = App::GetInstance().GetRenderer().GetD3DDevice()->CreateTexture2D(&desc, nullptr, &_windowFrame);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("创建 Texture2D 失败", hr));
			return false;
		}

		hr = _windowFrame.As<IDXGISurface1>(&_windowFrameSurface);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("从 Texture2D 获取 IDXGISurface1 失败", hr));
			return false;
		}

		// 获取客户区的坐标差值
		HDC hdcSrcClient = GetDCEx(hwndSrcClient, NULL, DCX_LOCKWINDOWUPDATE);
		if (!hdcSrcClient) {
			SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetDCEx 失败"));
			return false;
		}

		HDC hdcSrc = GetDCEx(hwndSrc, NULL, DCX_LOCKWINDOWUPDATE);
		if (!hdcSrc) {
			SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetDCEx 失败"));
			ReleaseDC(hwndSrc, hdcSrcClient);
			return false;
		}

		Utils::ScopeExit se([hwndSrc, hwndSrcClient, hdcSrcClient, hdcSrc]() {
			ReleaseDC(hwndSrc, hdcSrc);
			ReleaseDC(hwndSrcClient, hdcSrcClient);
		});

		POINT ptClient{}, ptWindow{};
		if (!GetDCOrgEx(hdcSrcClient, &ptClient)) {
			SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetDCOrgEx 失败"));
			return false;
		}
		if (!GetDCOrgEx(hdcSrc, &ptWindow)) {
			SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetDCOrgEx 失败"));
			return false;
		}

		_clientRect.left = ptClient.x - ptWindow.x;
		_clientRect.top = ptClient.y - ptWindow.y;
		_clientRect.right = _clientRect.left + clientSize.cx;
		_clientRect.bottom = _clientRect.top + clientSize.cy;
		_clientRect.front = 0;
		_clientRect.back = 1;
	}

	SPDLOG_LOGGER_INFO(logger, "PrintWindowFrameSource 初始化完成");
}

ComPtr<ID3D11Texture2D> PrintWindowFrameSource::GetOutput() {
	return _output;
}

bool PrintWindowFrameSource::Update() {
	if (_clientRect.back == 0) {
		HDC hdcDest;
		HRESULT hr = _outputSurface->GetDC(TRUE, &hdcDest);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("从 Texture2D 获取 IDXGISurface1 失败", hr));
			return false;
		}

		// https://chromium.googlesource.com/chromium/src.git/+/refs/heads/main/ui/snapshot/snapshot_win.cc
		// 不支持子窗口
		if (!PrintWindow(App::GetInstance().GetHwndSrc(), hdcDest, PW_CLIENTONLY | PW_RENDERFULLCONTENT)) {
			SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("PrintWindow 失败"));
		}

		_outputSurface->ReleaseDC(nullptr);
	} else {
		// srcClientRect 和 frameRect 不同，因此需要裁剪

		HDC hdcDest;
		HRESULT hr = _windowFrameSurface->GetDC(TRUE, &hdcDest);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("从 Texture2D 获取 IDXGISurface1 失败", hr));
			return false;
		}

		if (!PrintWindow(App::GetInstance().GetHwndSrc(), hdcDest, PW_CLIENTONLY | PW_RENDERFULLCONTENT)) {
			SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("PrintWindow 失败"));
		}

		_windowFrameSurface->ReleaseDC(nullptr);

		App::GetInstance().GetRenderer().GetD3DDC()->CopySubresourceRegion(_output.Get(),
			0, 0, 0, 0, _windowFrame.Get(), 0, &_clientRect);
	}

	return true;
}
