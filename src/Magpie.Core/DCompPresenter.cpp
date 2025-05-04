#include "pch.h"
#include "DCompPresenter.h"
#include "Logger.h"
#include "DeviceResources.h"
#include "ScalingWindow.h"
#include "Win32Helper.h"

namespace Magpie {

bool DCompPresenter::_Initialize(HWND hwndAttach) noexcept {
	ID3D11Device5* d3dDevice = _deviceResources->GetD3DDevice();

	HRESULT hr = DCompositionCreateDevice3(d3dDevice, IID_PPV_ARGS(&_dcompDevice));
	if (FAILED(hr)) {
		Logger::Get().ComError("DCompositionCreateDevice3 失败", hr);
		return false;
	}

	hr = _dcompDevice->CreateTargetForHwnd(hwndAttach, TRUE, _dcompTarget.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateTargetForHwnd 失败", hr);
		return false;
	}

	hr = _dcompDevice->CreateVisual(_dcompVisual.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateVisual 失败", hr);
		return false;
	}

	hr = _dcompTarget->SetRoot(_dcompVisual.get());
	if (FAILED(hr)) {
		Logger::Get().ComError("SetRoot 失败", hr);
		return false;
	}

	return _CreateSurface();
}

void DCompPresenter::_EndDraw() noexcept {
	_dcompSurface->EndDraw();
}

bool DCompPresenter::BeginFrame(
	winrt::com_ptr<ID3D11Texture2D>& frameTex,
	winrt::com_ptr<ID3D11RenderTargetView>& frameRtv,
	POINT& drawOffset
)  noexcept {
	HRESULT hr = _dcompSurface->BeginDraw(nullptr, IID_PPV_ARGS(&frameTex), &drawOffset);
	if (FAILED(hr)) {
		Logger::Get().ComError("BeginDraw 失败", hr);
		return false;
	}

	hr = _deviceResources->GetD3DDevice()->CreateRenderTargetView(
		frameTex.get(), nullptr, frameRtv.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateRenderTargetView 失败", hr);
		return false;
	}

	return true;
}

void DCompPresenter::_Present() noexcept {
	_dcompDevice->Commit();
}

bool DCompPresenter::_Resize() noexcept {
	// 先释放旧表面
	_dcompVisual->SetContent(nullptr);
	_dcompSurface = nullptr;

	return _CreateSurface();
}

bool DCompPresenter::_CreateSurface() noexcept {
	const SIZE rendererSize = Win32Helper::GetSizeOfRect(ScalingWindow::Get().RendererRect());
	HRESULT hr = _dcompDevice->CreateSurface(
		(UINT)rendererSize.cx,
		(UINT)rendererSize.cy,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_ALPHA_MODE_IGNORE,
		_dcompSurface.put()
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateSurface 失败", hr);
		return false;
	}

	hr = _dcompVisual->SetContent(_dcompSurface.get());
	if (FAILED(hr)) {
		Logger::Get().ComError("SetContent 失败", hr);
		return false;
	}

	return true;
}

}
