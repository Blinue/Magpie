#include "pch.h"
#include "DCompPresenter.h"
#include "Logger.h"
#include "DeviceResources.h"
#include "ScalingWindow.h"

namespace Magpie {

bool DCompPresenter::_Initialize(HWND hwndAttach) noexcept {
	ID3D11Device5* d3dDevice = _deviceResources->GetD3DDevice();

	HRESULT hr = DCompositionCreateDevice3(d3dDevice, IID_PPV_ARGS(&_device));
	if (FAILED(hr)) {
		Logger::Get().ComError("DCompositionCreateDevice3 失败", hr);
		return false;
	}

	hr = _device->CreateTargetForHwnd(hwndAttach, TRUE, _target.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateTargetForHwnd 失败", hr);
		return false;
	}

	hr = _device->CreateVisual(_visual.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateVisual 失败", hr);
		return false;
	}

	hr = _target->SetRoot(_visual.get());
	if (FAILED(hr)) {
		Logger::Get().ComError("SetRoot 失败", hr);
		return false;
	}

	return _CreateSurface();
}

void DCompPresenter::_EndDraw() noexcept {
	_surface->EndDraw();
}

winrt::com_ptr<ID3D11RenderTargetView> DCompPresenter::BeginFrame(POINT& updateOffset) noexcept {
	winrt::com_ptr<ID3D11Texture2D> frameTex;
	HRESULT hr = _surface->BeginDraw(nullptr, IID_PPV_ARGS(&frameTex), &updateOffset);
	if (FAILED(hr)) {
		Logger::Get().ComError("BeginDraw 失败", hr);
		return nullptr;
	}

	ID3D11Device5* d3dDevice = _deviceResources->GetD3DDevice();

	winrt::com_ptr<ID3D11RenderTargetView> frameRtv;
	hr = d3dDevice->CreateRenderTargetView(frameTex.get(), nullptr, frameRtv.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateRenderTargetView 失败", hr);
		return nullptr;
	}

	return frameRtv;
}

void DCompPresenter::_Present() noexcept {
	_device->Commit();
}

bool DCompPresenter::_Resize() noexcept {
	// 先释放旧表面
	_visual->SetContent(nullptr);
	_surface = nullptr;

	return _CreateSurface();
}

bool DCompPresenter::_CreateSurface() noexcept {
	const RECT& rendererRect = ScalingWindow::Get().RendererRect();
	HRESULT hr = _device->CreateSurface(
		UINT(rendererRect.right - rendererRect.left),
		UINT(rendererRect.bottom - rendererRect.top),
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_ALPHA_MODE_IGNORE,
		_surface.put()
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateSurface 失败", hr);
		return false;
	}

	hr = _visual->SetContent(_surface.get());
	if (FAILED(hr)) {
		Logger::Get().ComError("SetContent 失败", hr);
		return false;
	}

	return true;
}

}
