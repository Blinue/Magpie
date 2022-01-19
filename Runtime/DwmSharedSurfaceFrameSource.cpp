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

	if (!App::GetInstance().UpdateSrcFrameRect()) {
		SPDLOG_LOGGER_ERROR(logger, "UpdateSrcFrameRect 失败");
		return false;
	}
	
	HWND hwndSrc = App::GetInstance().GetHwndSrc();

	double a, bx, by;
	if (!_GetMapToOriginDPI(hwndSrc, a, bx, by)) {
		SPDLOG_LOGGER_ERROR(logger, "_GetMapToOriginDPI 失败");
		App::GetInstance().SetErrorMsg(ErrorMessages::FAILED_TO_CAPTURE);
		return false;
	}

	SPDLOG_LOGGER_INFO(logger, fmt::format("源窗口 DPI 缩放为 {}", 1 / a));

	RECT srcFrameRect = App::GetInstance().GetSrcFrameRect();
	RECT frameRect = {
		std::lround(srcFrameRect.left * a + bx),
		std::lround(srcFrameRect.top * a + by),
		std::lround(srcFrameRect.right * a + bx),
		std::lround(srcFrameRect.bottom * a + by)
	};
	if (frameRect.left < 0 || frameRect.top < 0 || frameRect.right < 0 
		|| frameRect.bottom < 0 || frameRect.right - frameRect.left <= 0
		|| frameRect.bottom - frameRect.top <= 0
	) {
		SPDLOG_LOGGER_ERROR(logger, "裁剪失败");
		App::GetInstance().SetErrorMsg(ErrorMessages::FAILED_TO_CROP);
		return false;
	}

	_frameInWnd = {
		(UINT)frameRect.left,
		(UINT)frameRect.top,
		0,
		(UINT)frameRect.right,
		(UINT)frameRect.bottom,
		1
	};

	D3D11_TEXTURE2D_DESC desc{};
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.Width = frameRect.right - frameRect.left;
	desc.Height = frameRect.bottom - frameRect.top;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	HRESULT hr = App::GetInstance().GetRenderer().GetD3DDevice()->CreateTexture2D(&desc, nullptr, &_output);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_CRITICAL(logger, MakeComErrorMsg("创建 Texture2D 失败", hr));
		return false;
	}

	SPDLOG_LOGGER_INFO(logger, "DwmSharedSurfaceFrameSource 初始化完成");
	return true;
}

FrameSourceBase::UpdateState DwmSharedSurfaceFrameSource::Update() {
	HANDLE sharedTextureHandle = NULL;
	if (!_dwmGetDxSharedSurface(App::GetInstance().GetHwndSrc(),
		&sharedTextureHandle, nullptr, nullptr, nullptr, nullptr)
		|| !sharedTextureHandle
	) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("DwmGetDxSharedSurface 失败"));
		return UpdateState::Error;
	}

	ComPtr<ID3D11Texture2D> sharedTexture;
	HRESULT hr = App::GetInstance().GetRenderer().GetD3DDevice()
		->OpenSharedResource(sharedTextureHandle, IID_PPV_ARGS(&sharedTexture));
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("OpenSharedResource 失败", hr));
		return UpdateState::Error;
	}
	
	App::GetInstance().GetRenderer().GetD3DDC()
		->CopySubresourceRegion(_output.Get(), 0, 0, 0, 0, sharedTexture.Get(), 0, &_frameInWnd);

	return UpdateState::NewFrame;
}
