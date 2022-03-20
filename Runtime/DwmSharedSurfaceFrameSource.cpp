#include "pch.h"
#include "DwmSharedSurfaceFrameSource.h"
#include "App.h"
#include "DeviceResources.h"
#include "Logger.h"


bool DwmSharedSurfaceFrameSource::Initialize() {
	if (!FrameSourceBase::Initialize()) {
		Logger::Get().Error("初始化 FrameSourceBase 失败");
		return false;
	}

	HMODULE hUser32 = GetModuleHandle(L"user32.dll");
	if (!hUser32) {
		Logger::Get().Win32Error("获取 User32.dll 模块句柄失败");
		return false;
	}
	_dwmGetDxSharedSurface = (_DwmGetDxSharedSurfaceFunc*)GetProcAddress(hUser32, "DwmGetDxSharedSurface");

	if (!_dwmGetDxSharedSurface) {
		Logger::Get().Win32Error("获取函数 DwmGetDxSharedSurface 地址失败");
		return false;
	}

	if (!_UpdateSrcFrameRect()) {
		Logger::Get().Error("_UpdateSrcFrameRect 失败");
		return false;
	}
	
	HWND hwndSrc = App::Get().GetHwndSrc();

	double a, bx, by;
	if (!_GetMapToOriginDPI(hwndSrc, a, bx, by)) {
		Logger::Get().Error("_GetMapToOriginDPI 失败");
		App::Get().SetErrorMsg(ErrorMessages::FAILED_TO_CAPTURE);
		return false;
	}

	Logger::Get().Info(fmt::format("源窗口 DPI 缩放为 {}", 1 / a));

	RECT frameRect = {
		std::lround(_srcFrameRect.left * a + bx),
		std::lround(_srcFrameRect.top * a + by),
		std::lround(_srcFrameRect.right * a + bx),
		std::lround(_srcFrameRect.bottom * a + by)
	};
	if (frameRect.left < 0 || frameRect.top < 0 || frameRect.right < 0 
		|| frameRect.bottom < 0 || frameRect.right - frameRect.left <= 0
		|| frameRect.bottom - frameRect.top <= 0
	) {
		Logger::Get().Error("裁剪失败");
		App::Get().SetErrorMsg(ErrorMessages::FAILED_TO_CROP);
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

	_output = App::Get().GetDeviceResources().CreateTexture2D(
		DXGI_FORMAT_B8G8R8A8_UNORM,
		frameRect.right - frameRect.left,
		frameRect.bottom - frameRect.top,
		D3D11_BIND_SHADER_RESOURCE
	);
	if (!_output) {
		Logger::Get().Error("创建 Texture2D 失败");
		return false;
	}

	Logger::Get().Info("DwmSharedSurfaceFrameSource 初始化完成");
	return true;
}

FrameSourceBase::UpdateState DwmSharedSurfaceFrameSource::Update() {
	HANDLE sharedTextureHandle = NULL;
	if (!_dwmGetDxSharedSurface(App::Get().GetHwndSrc(),
		&sharedTextureHandle, nullptr, nullptr, nullptr, nullptr)
		|| !sharedTextureHandle
	) {
		Logger::Get().Win32Error("DwmGetDxSharedSurface 失败");
		return UpdateState::Error;
	}

	winrt::com_ptr<ID3D11Texture2D> sharedTexture;
	HRESULT hr = App::Get().GetDeviceResources().GetD3DDevice()
		->OpenSharedResource(sharedTextureHandle, IID_PPV_ARGS(&sharedTexture));
	if (FAILED(hr)) {
		Logger::Get().ComError("OpenSharedResource 失败", hr);
		return UpdateState::Error;
	}
	
	App::Get().GetDeviceResources().GetD3DDC()
		->CopySubresourceRegion(_output.get(), 0, 0, 0, 0, sharedTexture.get(), 0, &_frameInWnd);

	return UpdateState::NewFrame;
}
