#include "pch.h"
#include "DCompPresenter.h"
#include "Logger.h"
#include "DeviceResources.h"
#include "ScalingWindow.h"

namespace Magpie {

bool DCompPresenter::Initialize(HWND hwndAttach, const DeviceResources& deviceResources) noexcept {
	_deviceResources = &deviceResources;

	ID3D11Device5* d3dDevice = deviceResources.GetD3DDevice();

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

	const RECT& rendererRect = ScalingWindow::Get().RendererRect();
	hr = _device->CreateSurface(
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

	hr = d3dDevice->CreateFence(
		_fenceValue, D3D11_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));
	if (FAILED(hr)) {
		// GH#979
		// 这个错误会在某些很旧的显卡上出现，似乎是驱动的 bug。文档中提到 ID3D11Device5::CreateFence 
		// 和 ID3D12Device::CreateFence 等价，但支持 DX12 的显卡也有失败的可能，如 GH#1013
		Logger::Get().ComError("CreateFence 失败", hr);
		return false;
	}

	if (!_fenceEvent.try_create(wil::EventOptions::None, nullptr)) {
		Logger::Get().Win32Error("CreateEvent 失败");
		return false;
	}

	return true;
}

winrt::com_ptr<ID3D11RenderTargetView> DCompPresenter::BeginFrame(POINT& updateOffset) noexcept {
	winrt::com_ptr<ID3D11Texture2D> frameTex;
	HRESULT hr = _surface->BeginDraw(nullptr, IID_PPV_ARGS(&frameTex), &updateOffset);
	if (FAILED(hr)) {
		return nullptr;
	}

	ID3D11Device5* d3dDevice = _deviceResources->GetD3DDevice();
	winrt::com_ptr<ID3D11RenderTargetView> frameRtv;
	d3dDevice->CreateRenderTargetView(frameTex.get(), nullptr, frameRtv.put());
	return frameRtv;
}

void DCompPresenter::EndFrame() noexcept {
	_surface->EndDraw();

	if (_isResized) {
		_isResized = false;

		ID3D11DeviceContext4* d3dDC = _deviceResources->GetD3DDC();

		// 等待渲染完成
		HRESULT hr = d3dDC->Signal(_fence.get(), ++_fenceValue);
		if (FAILED(hr)) {
			return;
		}

		hr = _fence->SetEventOnCompletion(_fenceValue, _fenceEvent.get());
		if (FAILED(hr)) {
			return;
		}

		d3dDC->Flush();

		WaitForSingleObject(_fenceEvent.get(), 1000);

		// 等待 DWM 开始合成下一帧
		_WaitForDwmComposition();
	}

	_device->Commit();
}

bool DCompPresenter::Resize() noexcept {
	_isResized = true;

	// 先释放旧表面
	_visual->SetContent(nullptr);
	_surface = nullptr;

	const RECT& rendererRect = ScalingWindow::Get().RendererRect();
	_device->CreateSurface(
		UINT(rendererRect.right - rendererRect.left),
		UINT(rendererRect.bottom - rendererRect.top),
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_ALPHA_MODE_IGNORE,
		_surface.put()
	);
	_visual->SetContent(_surface.get());

	return true;
}

}
