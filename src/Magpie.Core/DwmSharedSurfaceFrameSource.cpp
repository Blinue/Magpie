#include "pch.h"
#include "DwmSharedSurfaceFrameSource.h"
#include "Logger.h"
#include "ScalingWindow.h"
#include "DirectXHelper.h"
#include "DeviceResources.h"

namespace Magpie {

using DwmGetDxSharedSurfaceFunc = BOOL(
	HWND hWnd,
	HANDLE* phSurface,
	LUID* pAdapterLuid,
	ULONG* pFmtWindow,
	ULONG* pPresentFlags,
	ULONGLONG* pWin32KUpdateId
);

static DwmGetDxSharedSurfaceFunc* dwmGetDxSharedSurface = nullptr;

bool DwmSharedSurfaceFrameSource::_Initialize() noexcept {
	if (!dwmGetDxSharedSurface) {
		HMODULE hUser32 = GetModuleHandle(L"user32.dll");
		assert(hUser32);
		dwmGetDxSharedSurface = (DwmGetDxSharedSurfaceFunc*)GetProcAddress(hUser32, "DwmGetDxSharedSurface");

		if (!dwmGetDxSharedSurface) {
			Logger::Get().Win32Error("获取函数 DwmGetDxSharedSurface 地址失败");
			return false;
		}
	}

	const SrcInfo& srcInfo = ScalingWindow::Get().SrcInfo();
	
	RECT frameRect;
	double a, bx, by;
	if (!_GetMapToOriginDPI(srcInfo.Handle(), a, bx, by)) {
		// 很可能是因为窗口没有重定向表面，这种情况下 DwmSharedSurface 捕获肯定失败
		Logger::Get().Error("_GetMapToOriginDPI 失败");
		return false;
	}

	Logger::Get().Info(fmt::format("源窗口 DPI 缩放为 {}", 1 / a));

	const RECT& srcRect = srcInfo.FrameRect();
	frameRect = RECT{
		std::lround(srcRect.left * a + bx),
		std::lround(srcRect.top * a + by),
		std::lround(srcRect.right * a + bx),
		std::lround(srcRect.bottom * a + by)
	};
	
	if (frameRect.left < 0 || frameRect.top < 0 || frameRect.right < 0
		|| frameRect.bottom < 0 || frameRect.right - frameRect.left <= 0
		|| frameRect.bottom - frameRect.top <= 0
	) {
		Logger::Get().Error("裁剪失败");
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

	_output = DirectXHelper::CreateTexture2D(
		_deviceResources->GetD3DDevice(),
		DXGI_FORMAT_B8G8R8A8_UNORM,
		frameRect.right - frameRect.left,
		frameRect.bottom - frameRect.top,
		D3D11_BIND_SHADER_RESOURCE
	);
	if (!_output) {
		Logger::Get().Error("CreateTexture2D 失败");
		return false;
	}

	Logger::Get().Info("DwmSharedSurfaceFrameSource 初始化完成");
	return true;
}

FrameSourceState DwmSharedSurfaceFrameSource::_Update() noexcept {
	HANDLE sharedTextureHandle = NULL;
	if (!dwmGetDxSharedSurface(ScalingWindow::Get().SrcInfo().Handle(),
		&sharedTextureHandle, nullptr, nullptr, nullptr, nullptr)
		|| !sharedTextureHandle
	) {
		Logger::Get().Win32Error("DwmGetDxSharedSurface 失败");
		return FrameSourceState::Error;
	}

	winrt::com_ptr<ID3D11Texture2D> sharedTexture;
	HRESULT hr = _deviceResources->GetD3DDevice()
		->OpenSharedResource(sharedTextureHandle, IID_PPV_ARGS(&sharedTexture));
	if (FAILED(hr)) {
		Logger::Get().ComError("OpenSharedResource 失败", hr);
		return FrameSourceState::Error;
	}

	_deviceResources->GetD3DDC()->CopySubresourceRegion(
		_output.get(), 0, 0, 0, 0, sharedTexture.get(), 0, &_frameInWnd);

	return FrameSourceState::NewFrame;
}

}
