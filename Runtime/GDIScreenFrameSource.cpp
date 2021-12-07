#include "pch.h"
#include "GDIScreenFrameSource.h"
#include "App.h"


bool GDIScreenFrameSource::Initialize() {
	const RECT& srcClientRect = App::GetInstance().GetSrcClientRect();
	_frameSize = { srcClientRect.right - srcClientRect.left, srcClientRect.bottom - srcClientRect.top };

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
	
	SetWindowDisplayAffinity(App::GetInstance().GetHwndHost(), WDA_EXCLUDEFROMCAPTURE);

	SPDLOG_LOGGER_INFO(logger, "GDIScreenFrameSource 初始化完成");

	return true;
}

bool GDIScreenFrameSource::Update() {
	HDC hdcDest;
	HRESULT hr = _dxgiSurface->GetDC(TRUE, &hdcDest);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("从 Texture2D 获取 IDXGISurface1 失败", hr));
		return false;
	}

	const RECT& srcClientRect = App::GetInstance().GetSrcClientRect();

	_hdcScreen = GetDC(NULL);

	if (!BitBlt(hdcDest, 0, 0, _frameSize.cx, _frameSize.cy, _hdcScreen, srcClientRect.left, srcClientRect.top, SRCCOPY)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("BitBlt 失败"));
	}

	ReleaseDC(NULL, _hdcScreen);

	_dxgiSurface->ReleaseDC(nullptr);

	return true;
}
