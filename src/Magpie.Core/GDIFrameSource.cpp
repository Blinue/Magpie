#include "pch.h"
#include "GDIFrameSource.h"
#include "Logger.h"
#include "ScalingOptions.h"
#include "DirectXHelper.h"
#include "DeviceResources.h"
#include "ScalingWindow.h"

namespace Magpie {

bool GDIFrameSource::_Initialize() noexcept {
	const SrcInfo& srcInfo = ScalingWindow::Get().SrcInfo();

	double a, bx, by;
	if (!_GetMapToOriginDPI(srcInfo.Handle(), a, bx, by)) {
		// 很可能是因为窗口没有重定向表面，这种情况下 GDI 捕获肯定失败
		Logger::Get().Error("_GetMapToOriginDPI 失败");
		return false;
	}

	Logger::Get().Info(fmt::format("源窗口 DPI 缩放为 {}", 1 / a));

	_frameRect = {
		std::lround(srcInfo.FrameRect().left * a + bx),
		std::lround(srcInfo.FrameRect().top * a + by),
		std::lround(srcInfo.FrameRect().right * a + bx),
		std::lround(srcInfo.FrameRect().bottom * a + by)
	};

	if (_frameRect.left < 0 || _frameRect.top < 0 || _frameRect.right < 0
		|| _frameRect.bottom < 0 || _frameRect.right - _frameRect.left <= 0
		|| _frameRect.bottom - _frameRect.top <= 0
	) {
		Logger::Get().Error("裁剪失败");
		return false;
	}

	_output = DirectXHelper::CreateTexture2D(
		_deviceResources->GetD3DDevice(),
		DXGI_FORMAT_B8G8R8A8_UNORM,
		_frameRect.right - _frameRect.left,
		_frameRect.bottom - _frameRect.top,
		D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET,
		D3D11_USAGE_DEFAULT,
		D3D11_RESOURCE_MISC_GDI_COMPATIBLE
	);
	if (!_output) {
		Logger::Get().Error("创建纹理失败");
		return false;
	}

	_dxgiSurface = _output.try_as<IDXGISurface1>();
	if (!_dxgiSurface) {
		Logger::Get().Error("从 Texture2D 获取 IDXGISurface1 失败");
		return false;
	}

	Logger::Get().Info("GDIFrameSource 初始化完成");
	return true;
}

FrameSourceState GDIFrameSource::_Update() noexcept {
	HDC hdcDest;
	HRESULT hr = _dxgiSurface->GetDC(TRUE, &hdcDest);
	if (FAILED(hr)) {
		Logger::Get().ComError("从 Texture2D 获取 IDXGISurface1 失败", hr);
		return FrameSourceState::Error;
	}

	auto se = wil::scope_exit([&]() {
		_dxgiSurface->ReleaseDC(nullptr);
	});

	const HWND hwndSrc = ScalingWindow::Get().SrcInfo().Handle();
	wil::unique_hdc_window hdcSrc(wil::window_dc(GetDCEx(hwndSrc, NULL, DCX_WINDOW), hwndSrc));
	if (!hdcSrc) {
		Logger::Get().Win32Error("GetDC 失败");
		return FrameSourceState::Error;
	}

	if (!BitBlt(hdcDest, 0, 0, _frameRect.right - _frameRect.left, _frameRect.bottom - _frameRect.top,
		hdcSrc.get(), _frameRect.left, _frameRect.top, SRCCOPY)
	) {
		Logger::Get().Win32Error("BitBlt 失败");
		return FrameSourceState::Error;
	}

	return FrameSourceState::NewFrame;
}

}
