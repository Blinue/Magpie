#include "pch.h"
#include "DwmSharedSurfaceFrameSource.h"
#include "App.h"


extern std::shared_ptr<spdlog::logger> logger;


bool DwmSharedSurfaceFrameSource::Initialize(SIZE& frameSize) {
	_dwmGetDxSharedSurface = (_DwmGetDxSharedSurfaceFunc*)GetProcAddress(
		GetModuleHandle(L"user32.dll"), "DwmGetDxSharedSurface");

	if (!_dwmGetDxSharedSurface) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("获取函数 DwmGetDxSharedSurface 地址失败"));
		return false;
	}

	_d3dDC = App::GetInstance().GetRenderer().GetD3DDC();
	_d3dDevice = App::GetInstance().GetRenderer().GetD3DDevice();
	_hwndSrc = App::GetInstance().GetHwndSrc();

	POINT clientOffset;
	float dpiScale;
	{
		HDC hdcSrcClient = GetDCEx(_hwndSrc, NULL, DCX_LOCKWINDOWUPDATE);
		if (!hdcSrcClient) {
			return false;
		}
		HDC hdcSrcWindow = GetDCEx(_hwndSrc, NULL, DCX_LOCKWINDOWUPDATE | DCX_WINDOW);
		if (!hdcSrcWindow) {
			return false;
		}

		HBITMAP hBmpDest = (HBITMAP)GetCurrentObject(hdcSrcWindow, OBJ_BITMAP);
		if (!hBmpDest) {
			SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetCurrentObject 失败"));
			return false;
		}

		BITMAP bmp{};
		GetObject(hBmpDest, sizeof(bmp), &bmp);
		POINT p1{}, p2{};
		GetDCOrgEx(hdcSrcClient, &p1);
		GetDCOrgEx(hdcSrcWindow, &p2);
		ReleaseDC(_hwndSrc, hdcSrcClient);
		ReleaseDC(_hwndSrc, hdcSrcWindow);


		SIZE realWindowSize = { bmp.bmWidth, bmp.bmHeight };

		RECT srcWindowRect;
		if (!GetWindowRect(_hwndSrc, &srcWindowRect)) {
			SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetWindowRect 失败"));
			return false;
		}

		clientOffset = { p1.x - p2.x, p1.y - p2.y };
		dpiScale = float(srcWindowRect.right - srcWindowRect.left) / realWindowSize.cx;
	}
	

	RECT srcClientRect = App::GetInstance().GetSrcClientRect();
	frameSize = { 
		(LONG)ceilf((srcClientRect.right - srcClientRect.left) / dpiScale),
		(LONG)ceilf((srcClientRect.bottom - srcClientRect.top) / dpiScale)
	};
	
	_clientInFrame = {
		UINT(clientOffset.x),
		UINT(clientOffset.y),
		0,
		UINT(clientOffset.x + frameSize.cx),
		UINT(clientOffset.y + frameSize.cy),
		1
	};

	D3D11_TEXTURE2D_DESC desc{};
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.Width = frameSize.cx;
	desc.Height = frameSize.cy;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	HRESULT hr = _d3dDevice->CreateTexture2D(&desc, nullptr, &_output);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_CRITICAL(logger, MakeComErrorMsg("创建 Texture2D 失败", hr));
		return false;
	}

	SPDLOG_LOGGER_INFO(logger, "DwmSharedSurfaceFrameSource 初始化完成");
	return true;
}

ComPtr<ID3D11Texture2D> DwmSharedSurfaceFrameSource::GetOutput() {
	return _output;
}

bool DwmSharedSurfaceFrameSource::Update() {
	HANDLE sharedTextureHandle = NULL;
	if (!_dwmGetDxSharedSurface(_hwndSrc, &sharedTextureHandle, nullptr, nullptr, nullptr, nullptr)
		|| !sharedTextureHandle
	) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("DwmGetDxSharedSurface 失败"));
		return false;
	}

	ComPtr<ID3D11Texture2D> sharedTexture;
	HRESULT hr = _d3dDevice->OpenSharedResource(sharedTextureHandle, IID_PPV_ARGS(&sharedTexture));
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("OpenSharedResource 失败", hr));
		return false;
	}
	
	_d3dDC->CopySubresourceRegion(_output.Get(), 0, 0, 0, 0, sharedTexture.Get(), 0, &_clientInFrame);

	return true;
}
