#include "pch.h"
#include "PresenterBase.h"
#include "DeviceResources.h"
#include "Logger.h"
#include <dwmapi.h>

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

// 比 DwmFlush 更准确
static void WaitForDwmComposition() noexcept {
	LARGE_INTEGER qpf;
	QueryPerformanceFrequency(&qpf);
	qpf.QuadPart /= 10000000;

	DWM_TIMING_INFO info{};
	info.cbSize = sizeof(info);
	DwmGetCompositionTimingInfo(NULL, &info);

	LARGE_INTEGER time;
	QueryPerformanceCounter(&time);

	if (time.QuadPart >= (LONGLONG)info.qpcCompose) {
		return;
	}

	// 提前 1ms 结束然后忙等待
	time.QuadPart += 10000;
	if (time.QuadPart < (LONGLONG)info.qpcCompose) {
		LARGE_INTEGER liDueTime{
			.QuadPart = -((LONGLONG)info.qpcCompose - time.QuadPart) / qpf.QuadPart
		};
		static HANDLE timer = CreateWaitableTimerEx(nullptr, nullptr,
			CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
		SetWaitableTimerEx(timer, &liDueTime, 0, NULL, NULL, 0, 0);
		WaitForSingleObject(timer, INFINITE);
	} else {
		Sleep(0);
	}

	while (true) {
		QueryPerformanceCounter(&time);

		if (time.QuadPart >= (LONGLONG)info.qpcCompose) {
			return;
		}

		Sleep(0);
	}
}

void PresenterBase::_WaitForDwmAfterResize() noexcept {
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
	WaitForDwmComposition();
}

}
