#include "pch.h"
#include "DirectXHelper.h"
#include <dxgi1_6.h>
#include <Logger.h>

using namespace winrt;

namespace Magpie {

GraphicsCardId DirectXHelper::GetGraphicsCardIdFromIdx(int idx) noexcept {
	GraphicsCardId result;

	if (idx < 0) {
		// 使用默认显卡
		return result;
	}

	com_ptr<IDXGIFactory1> dxgiFactory;
	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateDXGIFactory1 失败", hr);
		return result;
	}

	com_ptr<IDXGIAdapter1> adapter;
	hr = dxgiFactory->EnumAdapters1(idx, adapter.put());
	if (FAILED(hr)) {
		// 可能因为该显卡已不存在
		Logger::Get().ComError("EnumAdapters1 失败", hr);
		return result;
	}

	DXGI_ADAPTER_DESC1 desc;
	hr = adapter->GetDesc1(&desc);
	if (FAILED(hr)) {
		Logger::Get().ComError("GetDesc1 失败", hr);
		return result;
	}

	if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
		// 不使用 WARP
		return result;
	}

	// 不检查 FL11，由 AdaptersService 检查
	result.idx = idx;
	result.vendorId = desc.VendorId;
	result.deviceId = desc.DeviceId;
	return result;
}

}
