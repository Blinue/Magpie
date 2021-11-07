#include "pch.h"
#include "DwmSharedSurfaceFrameSource.h"
#include "App.h"


extern std::shared_ptr<spdlog::logger> logger;


bool DwmSharedSurfaceFrameSource::Initialize() {
	_dwmGetDxSharedSurface = (_DwmGetDxSharedSurfaceFunc*)GetProcAddress(
		GetModuleHandle(L"user32.dll"), "DwmGetDxSharedSurface");

	if (!_dwmGetDxSharedSurface) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("获取函数 DwmGetDxSharedSurface 地址失败"));
		return false;
	}

	_d3dDC = App::GetInstance().GetRenderer().GetD3DDC();
	_d3dDevice = App::GetInstance().GetRenderer().GetD3DDevice();
	_hwndSrc = App::GetInstance().GetHwndSrc();
	
	SIZE frameSize;
	if (!_CalcFrameSize(frameSize)) {
		SPDLOG_LOGGER_ERROR(logger, "_CalcFrameSize 失败");
		return false;
	}

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
	
	_d3dDC->CopySubresourceRegion(_output.Get(), 0, 0, 0, 0, sharedTexture.Get(), 0, &_frameInWnd);

	return true;
}


bool DwmSharedSurfaceFrameSource::_CalcFrameSize(SIZE& frameSize) {
	// 首先尝试 DPI 感知方式，失败时回落到普通方式

	POINT clientOffset;
	float dpiScale;
	bool success = true;

	if ( !_GetWindowDpiScale(_hwndSrc, dpiScale)) {
		SPDLOG_LOGGER_ERROR(logger, "_GetWindowDpiScale 失败");
		success = false;
	}

	SPDLOG_LOGGER_INFO(logger, fmt::format("源窗口 DPI 缩放为 {}", dpiScale));

	if (abs(dpiScale - 1.0f) < 1e-5f) {
		// 无 DPI 缩放，使用普通方式
		success = false;
	}

	if (success && !_GetDpiAwareWindowClientOffset(_hwndSrc, clientOffset)) {
		SPDLOG_LOGGER_ERROR(logger, "_GetDpiAwareWindowClientOffset 失败");
		success = false;
	}
	
	if (success) {
		RECT srcClientRect = App::GetInstance().GetSrcClientRect();
		frameSize = {
			(LONG)ceilf((srcClientRect.right - srcClientRect.left) / dpiScale),
			(LONG)ceilf((srcClientRect.bottom - srcClientRect.top) / dpiScale)
		};

		_frameInWnd = {
			UINT(clientOffset.x),
			UINT(clientOffset.y),
			0,
			UINT(clientOffset.x + frameSize.cx),
			UINT(clientOffset.y + frameSize.cy),
			1
		};
	} else {
		// 回落到普通方式
		RECT srcWindowRect;
		if (!GetWindowRect(_hwndSrc, &srcWindowRect)) {
			SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetWindowRect 失败"));
			return false;
		}

		const RECT srcClientRect = App::GetInstance().GetSrcClientRect();
		_frameInWnd = {
			UINT(srcClientRect.left - srcWindowRect.left),
			UINT(srcClientRect.top - srcWindowRect.top),
			0,
			UINT(srcClientRect.right - srcWindowRect.left),
			UINT(srcClientRect.bottom - srcWindowRect.top),
			1
		};

		frameSize = { srcClientRect.right - srcClientRect.left, srcClientRect.bottom - srcClientRect.top };
	}
	
	return true;
}
