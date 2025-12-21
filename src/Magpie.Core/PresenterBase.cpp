#include "pch.h"
#include "PresenterBase.h"
#include "DeviceResources.h"
#include "Logger.h"
#include "ScalingWindow.h"

namespace Magpie {

bool PresenterBase::Initialize(HWND hwndAttach, const DeviceResources& deviceResources) noexcept {
	_deviceResources = &deviceResources;

	HRESULT hr = deviceResources.GetD3DDevice()->CreateFence(
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

	return _Initialize(hwndAttach);
}

uint32_t PresenterBase::_CalcBufferCount() noexcept {
	// 缓冲区数量取决于 ScalingRuntime::_ScalingThreadProc 中检查光标移动的频率
	return ScalingWindow::Get().Options().Is3DGameMode() ? 4 : 8;
}

void PresenterBase::_WaitForGpu() noexcept {
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
}

}
