#include "pch.h"
#include "GDIFrameSource.h"
#include "App.h"


extern std::shared_ptr<spdlog::logger> logger;

bool GDIFrameSource::Initialize() {
	_hwndSrc = App::GetInstance().GetHwndSrc();
	const RECT& srcClientRect = App::GetInstance().GetSrcClientRect();

	float dpiScale = -1;
	if (!_GetWindowDpiScale(_hwndSrc, dpiScale)) {
		SPDLOG_LOGGER_ERROR(logger, "_GetWindowDpiScale 失败");
	}

	SPDLOG_LOGGER_INFO(logger, fmt::format("源窗口 DPI 缩放为 {}", dpiScale));

	if (dpiScale > 0 && abs(dpiScale - 1.0f) > 1e-5f) {
		// DPI 感知
		_frameSize = {
			(LONG)ceilf((srcClientRect.right - srcClientRect.left) / dpiScale),
			(LONG)ceilf((srcClientRect.bottom - srcClientRect.top) / dpiScale)
		};
	} else {
		_frameSize = { srcClientRect.right - srcClientRect.left, srcClientRect.bottom - srcClientRect.top };
	}

	D3D11_TEXTURE2D_DESC desc{};
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.Width = _frameSize.cx;
	desc.Height = _frameSize.cy;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	desc.MiscFlags = D3D11_RESOURCE_MISC_GDI_COMPATIBLE;
	HRESULT hr = App::GetInstance().GetRenderer().GetD3DDevice()->CreateTexture2D(&desc, nullptr, &_output);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("创建 Texture2D 失败", hr));
		return false;
	}

	hr = _output.As<IDXGISurface1>(&_dxgiSurface);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("从 Texture2D 获取 IDXGISurface1 失败", hr));
		return false;
	}

	SPDLOG_LOGGER_INFO(logger, "GDIFrameSource 初始化完成");
	return true;
}

ComPtr<ID3D11Texture2D> GDIFrameSource::GetOutput() {
	return _output;
}

bool GDIFrameSource::Update() {
	HDC hdcDest;
	HRESULT hr = _dxgiSurface->GetDC(TRUE, &hdcDest);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("从 Texture2D 获取 IDXGISurface1 失败", hr));
		return false;
	}

	HDC hdcSrcClient = GetDCEx(_hwndSrc, NULL, DCX_LOCKWINDOWUPDATE);
	if (!hdcSrcClient) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetDC 失败"));
		_dxgiSurface->ReleaseDC(nullptr);
		return false;
	}

	if (!BitBlt(hdcDest, 0, 0, _frameSize.cx, _frameSize.cy, hdcSrcClient, 0, 0, SRCCOPY)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("BitBlt 失败"));
	}

	ReleaseDC(_hwndSrc, hdcSrcClient);
	_dxgiSurface->ReleaseDC(nullptr);

    return true;
}
