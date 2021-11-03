#include "pch.h"
#include "GDIOverDXGIFrameSource.h"
#include "App.h"


extern std::shared_ptr<spdlog::logger> logger;

bool GDIOverDXGIFrameSource::Initialize(SIZE& frameSize) {
	_hwndSrc = App::GetInstance().GetHwndSrc();
	const RECT& srcClientRect = App::GetInstance().GetSrcClientRect();

	D3D11_TEXTURE2D_DESC desc{};
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.Width = srcClientRect.right - srcClientRect.left;
	desc.Height = srcClientRect.bottom - srcClientRect.top;
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

	SPDLOG_LOGGER_INFO(logger, "GDIOverDXGIFrameSource 初始化完成");
	return true;
}

ComPtr<ID3D11Texture2D> GDIOverDXGIFrameSource::GetOutput() {
	return _output;
}

bool GDIOverDXGIFrameSource::Update() {
	HDC hdcDest;
	HRESULT hr = _dxgiSurface->GetDC(TRUE, &hdcDest);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("从 Texture2D 获取 IDXGISurface1 失败", hr));
		return false;
	}

	HDC hdcSrc = GetDCEx(_hwndSrc, NULL, DCX_LOCKWINDOWUPDATE);
	if (!hdcSrc) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetDC 失败"));
		_dxgiSurface->ReleaseDC(nullptr);
		return false;
	}

	const RECT& srcClientRect = App::GetInstance().GetSrcClientRect();
	if (!BitBlt(hdcDest, 0, 0, srcClientRect.right - srcClientRect.left, srcClientRect.bottom - srcClientRect.top, hdcSrc, 0, 0, SRCCOPY)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("BitBlt 失败"));
	}

	ReleaseDC(_hwndSrc, hdcSrc);
	_dxgiSurface->ReleaseDC(nullptr);

    return true;
}
