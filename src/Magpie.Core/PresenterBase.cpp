#include "pch.h"
#include "PresenterBase.h"
#include "DeviceResources.h"
#include "Logger.h"
#include "ScalingWindow.h"

namespace Magpie {

bool PresenterBase::Initialize(HWND hwndAttach, const DeviceResources& deviceResources) noexcept {
	_deviceResources = &deviceResources;

	if (!_gpuCompletion.Initialize(deviceResources.GetD3DDevice())) {
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

	if (_gpuCompletion.Signal(d3dDC)) {
		_gpuCompletion.Wait(d3dDC, 1000);
	}
}

}
