#include "pch.h"
#include "SharedSurfaceFrameSource.h"
#include "App.h"


extern std::shared_ptr<spdlog::logger> logger;

bool SharedSurfaceFrameSource::Initialize() {
	HMODULE user32 = GetModuleHandle(L"user32");
	if (!user32) {
		SPDLOG_LOGGER_ERROR(logger, "获取 User32 模块句柄失败");
		return false;
	}

	_dwmGetDxSharedSurface = (_DwmGetDxSharedSurfaceFunc*)GetProcAddress(user32, "DwmGetDxSharedSurface");

	if (!_dwmGetDxSharedSurface) {
		SPDLOG_LOGGER_ERROR(logger, "获取函数 DwmGetDxSharedSurfaceFunc 地址失败");
		return false;
	}

	_d3dDC = App::GetInstance().GetRenderer().GetD3DDC();
	_d3dDevice = App::GetInstance().GetRenderer().GetD3DDevice();
	_hwndSrc = App::GetInstance().GetHwndSrc();

	RECT srcWindowRect;
	GetWindowRect(_hwndSrc, &srcWindowRect);
	const RECT srcClientRect = App::GetInstance().GetSrcClientRect();
	
	_clientInFrame = {
		srcClientRect.left - srcWindowRect.left,
		srcClientRect.top - srcWindowRect.top,
		srcClientRect.right - srcWindowRect.left,
		srcClientRect.bottom - srcWindowRect.top
	};

	D3D11_TEXTURE2D_DESC desc{};
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.Width = srcClientRect.right - srcClientRect.left;
	desc.Height = srcClientRect.bottom - srcClientRect.top;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	_d3dDevice->CreateTexture2D(&desc, nullptr, &_output);

	return true;
}

ComPtr<ID3D11Texture2D> SharedSurfaceFrameSource::GetOutput() {
	return _output;
}

bool SharedSurfaceFrameSource::Update() {
	HANDLE handle = NULL;
	if (!_dwmGetDxSharedSurface(_hwndSrc, &handle, nullptr, nullptr, nullptr, nullptr)) {
		return false;
	}

	ComPtr<ID3D11Texture2D> texture;
	HRESULT hr = _d3dDevice->OpenSharedResource(handle, IID_PPV_ARGS(&texture));
	if (FAILED(hr)) {
		return false;
	}
	
	D3D11_BOX box{
		_clientInFrame.left, _clientInFrame.top, 0,
		_clientInFrame.right, _clientInFrame.bottom, 1
	};
	_d3dDC->CopySubresourceRegion(_output.Get(), 0, 0, 0, 0, texture.Get(), 0, &box);

	return true;
}
