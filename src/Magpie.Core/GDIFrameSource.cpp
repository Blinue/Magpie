#include "pch.h"
#include "GDIFrameSource.h"
#include "Logger.h"
#include "ScalingOptions.h"
#include "DirectXHelper.h"
#include "DeviceResources.h"
#include "ScalingWindow.h"

namespace Magpie::Core {

bool GDIFrameSource::_Initialize() noexcept {
	if (!_CalcSrcRect()) {
		return false;
	}

	const HWND hwndSrc = ScalingWindow::Get().HwndSrc();

	double a, bx, by;
	if (_GetMapToOriginDPI(hwndSrc, a, bx, by)) {
		Logger::Get().Info(fmt::format("源窗口 DPI 缩放为 {}", 1 / a));

		_frameRect = {
			std::lround(_srcRect.left * a + bx),
			std::lround(_srcRect.top * a + by),
			std::lround(_srcRect.right * a + bx),
			std::lround(_srcRect.bottom * a + by)
		};
	} else {
		Logger::Get().Error("_GetMapToOriginDPI 失败");

		// _GetMapToOriginDPI 失败则假设 DPI 缩放为 1
		RECT srcWindowRect;
		if (!GetWindowRect(hwndSrc, &srcWindowRect)) {
			Logger::Get().Win32Error("GetWindowRect 失败");
			return false;
		}

		_frameRect = {
			_srcRect.left - srcWindowRect.left,
			_srcRect.top - srcWindowRect.top,
			_srcRect.right - srcWindowRect.left,
			_srcRect.bottom - srcWindowRect.top
		};
	}

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

FrameSourceBase::UpdateState GDIFrameSource::_Update() noexcept {
	HDC hdcDest;
	HRESULT hr = _dxgiSurface->GetDC(TRUE, &hdcDest);
	if (FAILED(hr)) {
		Logger::Get().ComError("从 Texture2D 获取 IDXGISurface1 失败", hr);
		return UpdateState::Error;
	}

	auto se = wil::scope_exit([&]() {
		_dxgiSurface->ReleaseDC(nullptr);
	});

	const HWND hwndSrc = ScalingWindow::Get().HwndSrc();
	wil::unique_hdc_window hdcSrc(
		wil::window_dc(GetDCEx(hwndSrc, NULL, DCX_LOCKWINDOWUPDATE | DCX_WINDOW), hwndSrc));
	if (!hdcSrc) {
		Logger::Get().Win32Error("GetDC 失败");
		return UpdateState::Error;
	}

	if (!BitBlt(hdcDest, 0, 0, _frameRect.right - _frameRect.left, _frameRect.bottom - _frameRect.top,
		hdcSrc.get(), _frameRect.left, _frameRect.top, SRCCOPY)
	) {
		Logger::Get().Win32Error("BitBlt 失败");
	}

	return UpdateState::NewFrame;
}

}
