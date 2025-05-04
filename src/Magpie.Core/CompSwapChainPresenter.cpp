#include "pch.h"
#include "CompSwapChainPresenter.h"
#include "DeviceResources.h"
#include "Logger.h"
#include "Win32Helper.h"
#include "ScalingWindow.h"

namespace Magpie {

static winrt::com_ptr<IPresentationFactory> CreatePresentationFactory(ID3D11Device* d3dDevice) noexcept {
	HMODULE hDcomp = GetModuleHandle(L"dcomp.dll");
	assert(hDcomp);
	auto func = (decltype(::CreatePresentationFactory)*)GetProcAddress(
		hDcomp, "CreatePresentationFactory");

	winrt::com_ptr<IPresentationFactory> result;
	HRESULT hr = func(d3dDevice, IID_PPV_ARGS(&result));
	if (FAILED(hr)) {
		Logger::Get().ComError("CreatePresentationFactory 失败", hr);
	}

	return result;
}

bool CompSwapChainPresenter::_Initialize(HWND hwndAttach) noexcept {
	if (!Win32Helper::GetOSVersion().IsWin11()) {
		Logger::Get().Error("OS 不支持 composition swapchain");
		return false;
	}

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

	winrt::com_ptr<IPresentationFactory> presentationFactory =
		CreatePresentationFactory(d3dDevice);
	if (!presentationFactory) {
		Logger::Get().Error("CreatePresentationFactory 失败");
		return false;
	}

	if (!presentationFactory->IsPresentationSupported()) {
		Logger::Get().Error("此 D3D 设备不支持 composition swapchain");
		return false;
	}

	if (!presentationFactory->IsPresentationSupportedWithIndependentFlip()) {
		Logger::Get().Info("此 D3D 设备不支持 independent flip");
	}

	hr = presentationFactory->CreatePresentationManager(_presentationManager.put());
	if (FAILED(hr)) {
		return false;
	}

	wil::unique_handle hCompSurface;
	hr = DCompositionCreateSurfaceHandle(
		COMPOSITIONOBJECT_ALL_ACCESS,
		nullptr,
		hCompSurface.put()
	);
	if (FAILED(hr)) {
		return false;
	}

	hr = _presentationManager->CreatePresentationSurface(
		hCompSurface.get(), _presentationSurface.put());
	if (FAILED(hr)) {
		return false;
	}

	winrt::com_ptr<IUnknown> compSurface;
	hr = _dcompDevice->CreateSurfaceFromHandle(hCompSurface.get(), compSurface.put());
	if (FAILED(hr)) {
		return false;
	}

	hr = _dcompVisual->SetContent(compSurface.get());
	if (FAILED(hr)) {
		return false;
	}

	hr = _dcompDevice->Commit();
	if (FAILED(hr)) {
		return false;
	}

	hr = _presentationManager->GetPresentRetiringFence(IID_PPV_ARGS(&_presentationFence));
	if (FAILED(hr)) {
		return false;
	}

	return true;
}

void CompSwapChainPresenter::_Present() noexcept {
	_presentationManager->Present();
}

bool CompSwapChainPresenter::_Resize() noexcept {
	_presentationBuffers = {};
	_bufferTextures = {};
	_bufferRtvs = {};
	return true;
}

bool CompSwapChainPresenter::BeginFrame(
	winrt::com_ptr<ID3D11Texture2D>& frameTex,
	winrt::com_ptr<ID3D11RenderTargetView>& frameRtv,
	POINT& drawOffset
) noexcept {
	if (UINT64 nextId = _presentationManager->GetNextPresentId(); nextId >= 2) {
		_presentationFence->SetEventOnCompletion(nextId - 2, _fenceEvent.get());
		WaitForSingleObject(_fenceEvent.get(), 1000);
	}

	_curBufferIdx = (_curBufferIdx + 1) % 2;
	winrt::com_ptr<ID3D11Texture2D>& curTex = _bufferTextures[_curBufferIdx];

	if (!curTex) {
		const SIZE rendererSize = Win32Helper::GetSizeOfRect(ScalingWindow::Get().RendererRect());

		D3D11_TEXTURE2D_DESC desc{};
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Width = (UINT)rendererSize.cx;
		desc.Height = (UINT)rendererSize.cy;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		desc.MiscFlags =
			D3D11_RESOURCE_MISC_SHARED |
			D3D11_RESOURCE_MISC_SHARED_NTHANDLE |
			D3D11_RESOURCE_MISC_SHARED_DISPLAYABLE;

		HRESULT hr = _deviceResources->GetD3DDevice()->CreateTexture2D(
			&desc, nullptr, curTex.put());
		if (FAILED(hr)) {
			return false;
		}

		hr = _presentationManager->AddBufferFromResource(
			curTex.get(), _presentationBuffers[_curBufferIdx].put());
		if (FAILED(hr)) {
			return false;
		}

		RECT srcRect{ 0,0,rendererSize.cx,rendererSize.cy };
		_presentationSurface->SetSourceRect(&srcRect);
	}

	boolean available = false;
	_presentationBuffers[_curBufferIdx]->IsAvailable(&available);
	if (!available) {
		assert(false);
		return false;
	}

	HRESULT hr = _presentationSurface->SetBuffer(_presentationBuffers[_curBufferIdx].get());
	if (FAILED(hr)) {
		return false;
	}

	winrt::com_ptr<ID3D11RenderTargetView>& curRtv = _bufferRtvs[_curBufferIdx];
	if (!curRtv) {
		hr = _deviceResources->GetD3DDevice()->CreateRenderTargetView(
			curTex.get(), nullptr, curRtv.put());
		if (FAILED(hr)) {
			return false;
		}
	}

	drawOffset = {};
	frameTex = curTex;
	frameRtv = curRtv;
	return true;
}

}
