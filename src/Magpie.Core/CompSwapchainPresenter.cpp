#include "pch.h"
#include "CompSwapchainPresenter.h"
#include "DeviceResources.h"
#include "Logger.h"
#include "ScalingWindow.h"
#include "Win32Helper.h"

namespace Magpie {

static winrt::com_ptr<IPresentationFactory> CreatePresentationFactory(ID3D11Device* d3dDevice) noexcept {
	winrt::com_ptr<IPresentationFactory> result;

	static const auto createPresentationFactory =
		Win32Helper::LoadSystemFunction<decltype(::CreatePresentationFactory)>(
		L"dcomp.dll", "CreatePresentationFactory");
	if (!createPresentationFactory) {
		return result;
	}
	
	HRESULT hr = createPresentationFactory(d3dDevice, IID_PPV_ARGS(&result));
	if (FAILED(hr)) {
		Logger::Get().ComError("CreatePresentationFactory 失败", hr);
	}

	return result;
}

bool CompSwapchainPresenter::_Initialize(HWND hwndAttach) noexcept {
	if (Win32Helper::GetOSVersion().IsWin10()) {
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
		Logger::Get().ComError("CreatePresentationManager 失败", hr);
		return false;
	}

	wil::unique_handle hCompSurface;
	hr = DCompositionCreateSurfaceHandle(
		COMPOSITIONOBJECT_ALL_ACCESS,
		nullptr,
		hCompSurface.put()
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("DCompositionCreateSurfaceHandle 失败", hr);
		return false;
	}

	hr = _presentationManager->CreatePresentationSurface(
		hCompSurface.get(), _presentationSurface.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreatePresentationSurface 失败", hr);
		return false;
	}

	winrt::com_ptr<IUnknown> compSurface;
	hr = _dcompDevice->CreateSurfaceFromHandle(hCompSurface.get(), compSurface.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateSurfaceFromHandle 失败", hr);
		return false;
	}

	hr = _dcompVisual->SetContent(compSurface.get());
	if (FAILED(hr)) {
		Logger::Get().ComError("SetContent 失败", hr);
		return false;
	}

	hr = _dcompDevice->Commit();
	if (FAILED(hr)) {
		Logger::Get().ComError("Commit 失败", hr);
		return false;
	}

	hr = _presentationManager->GetPresentRetiringFence(IID_PPV_ARGS(&_presentationFence));
	if (FAILED(hr)) {
		Logger::Get().ComError("GetPresentRetiringFence 失败", hr);
		return false;
	}

	const uint32_t bufferCount = _CalcBufferCount();
	_presentationBuffers.resize(bufferCount);
	_presentationBufferAvailableEvents.resize(bufferCount);
	_bufferTextures.resize(bufferCount);
	_bufferRtvs.resize(bufferCount);

	return true;
}

bool CompSwapchainPresenter::BeginFrame(
	winrt::com_ptr<ID3D11Texture2D>& frameTex,
	winrt::com_ptr<ID3D11RenderTargetView>& frameRtv,
	POINT& drawOffset
) noexcept {
	// 寻找可用的缓冲区
	uint32_t curIdx = std::numeric_limits<uint32_t>::max();

	// 先寻找未初始化的缓冲区
	const uint32_t bufferCount = (uint32_t)_presentationBuffers.size();
	for (uint32_t i = 0; i < bufferCount; ++i) {
		if (_presentationBuffers[i]) {
			continue;
		}

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
			&desc, nullptr, _bufferTextures[i].put());
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateTexture2D 失败", hr);
			return false;
		}

		hr = _presentationManager->AddBufferFromResource(
			_bufferTextures[i].get(), _presentationBuffers[i].put());
		if (FAILED(hr)) {
			Logger::Get().ComError("AddBufferFromResource 失败", hr);
			return false;
		}

		hr = _presentationBuffers[i]->GetAvailableEvent(
			_presentationBufferAvailableEvents[i].put());
		if (FAILED(hr)) {
			Logger::Get().ComError("GetAvailableEvent 失败", hr);
			return false;
		}

		RECT srcRect{ 0,0,rendererSize.cx,rendererSize.cy };
		hr = _presentationSurface->SetSourceRect(&srcRect);
		if (FAILED(hr)) {
			Logger::Get().ComError("SetSourceRect 失败", hr);
			return false;
		}

		curIdx = i;
		break;
	}

	if (curIdx == std::numeric_limits<uint32_t>::max()) {
		// 等待某个缓冲区空闲
		DWORD waitResult = WaitForMultipleObjects(
			bufferCount, (HANDLE*)_presentationBufferAvailableEvents.data(), FALSE, INFINITE);
		if (waitResult < WAIT_OBJECT_0 || waitResult > WAIT_OBJECT_0 + bufferCount - 1) {
			Logger::Get().Error("WaitForMultipleObjects 失败");
			return false;
		}

		curIdx = waitResult - WAIT_OBJECT_0;
	}

	HRESULT hr = _presentationSurface->SetBuffer(_presentationBuffers[curIdx].get());
	if (FAILED(hr)) {
		Logger::Get().ComError("SetBuffer 失败", hr);
		return false;
	}

	winrt::com_ptr<ID3D11RenderTargetView>& curRtv = _bufferRtvs[curIdx];
	if (!curRtv) {
		hr = _deviceResources->GetD3DDevice()->CreateRenderTargetView(
			_bufferTextures[curIdx].get(), nullptr, curRtv.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateRenderTargetView 失败", hr);
			return false;
		}
	}

	drawOffset = {};
	frameTex = _bufferTextures[curIdx];
	frameRtv = curRtv;

	return true;
}

void CompSwapchainPresenter::EndFrame(bool waitForRenderComplete) noexcept {
	if (waitForRenderComplete || _isResized) {
		// 下面两个调用用于减少调整窗口尺寸时的边缘闪烁，参见 AdaptivePresenter::EndFrame

		// 等待渲染完成
		_WaitForRenderComplete();

		// 等待 DWM 开始合成新一帧
		_WaitForDwmComposition();
	}

	_presentationManager->Present();

	if (_isResized) {
		_isResized = false;
	} else {
		// 确保前一帧渲染完成再渲染下一帧，既降低了 GPU 负载，也能降低延迟
		_WaitForRenderComplete();
	}
}

bool CompSwapchainPresenter::OnResize() noexcept {
	_isResized = true;

	// 缓冲区在 BeginFrame 中按需创建
	std::fill(_presentationBuffers.begin(), _presentationBuffers.end(), nullptr);
	std::fill(_presentationBufferAvailableEvents.begin(),
		_presentationBufferAvailableEvents.end(), nullptr);
	std::fill(_bufferTextures.begin(), _bufferTextures.end(), nullptr);
	std::fill(_bufferRtvs.begin(), _bufferRtvs.end(), nullptr);

	return true;
}

}
