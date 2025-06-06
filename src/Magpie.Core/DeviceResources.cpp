#include "pch.h"
#include "DeviceResources.h"
#include "ScalingOptions.h"
#include "Logger.h"
#include "StrHelper.h"
#include "DirectXHelper.h"
#include "ScalingWindow.h"

namespace Magpie {

bool DeviceResources::Initialize() noexcept {
#ifdef _DEBUG
	UINT flag = DXGI_CREATE_FACTORY_DEBUG;
#else
	UINT flag = 0;
#endif

	HRESULT hr = CreateDXGIFactory2(flag, IID_PPV_ARGS(_dxgiFactory.put()));
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateDXGIFactory2 失败", hr);
		return false;
	}

	// 检查可变帧率支持
	BOOL supportTearing = FALSE;
	hr = _dxgiFactory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &supportTearing, sizeof(supportTearing));
	if (FAILED(hr)) {
		Logger::Get().ComWarn("CheckFeatureSupport 失败", hr);
	}

	_isTearingSupported = supportTearing;
	Logger::Get().Info(fmt::format("可变刷新率支持: {}", supportTearing ? "是" : "否"));

	if (!_ObtainAdapterAndDevice(ScalingWindow::Get().Options().graphicsCardId)) {
		Logger::Get().Error("找不到可用的图形适配器");
		return false;
	}

	return true;
}

ID3D11SamplerState* DeviceResources::GetSampler(D3D11_FILTER filterMode, D3D11_TEXTURE_ADDRESS_MODE addressMode) noexcept {
	auto key = std::make_pair(filterMode, addressMode);
	auto it = _samMap.find(key);
	if (it != _samMap.end()) {
		return it->second.get();
	}

	winrt::com_ptr<ID3D11SamplerState> sam;

	D3D11_SAMPLER_DESC desc{
		.Filter = filterMode,
		.AddressU = addressMode,
		.AddressV = addressMode,
		.AddressW = addressMode,
		.ComparisonFunc = D3D11_COMPARISON_NEVER
	};
	HRESULT hr = _d3dDevice->CreateSamplerState(&desc, sam.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("创建 ID3D11SamplerState 出错", hr);
		return nullptr;
	}

	return _samMap.emplace(key, std::move(sam)).first->second.get();
}

bool DeviceResources::_ObtainAdapterAndDevice(GraphicsCardId graphicsCardId) noexcept {
	winrt::com_ptr<IDXGIAdapter1> adapter;
	// 记录不支持 FL11 的显卡索引，防止重复尝试
	int failedIdx = -1;

	if (graphicsCardId.idx >= 0) {
		assert(graphicsCardId.vendorId != 0 && graphicsCardId.deviceId != 0);
		
		// 先使用索引
		HRESULT hr = _dxgiFactory->EnumAdapters1(graphicsCardId.idx, adapter.put());
		if (SUCCEEDED(hr)) {
			DXGI_ADAPTER_DESC1 desc;
			hr = adapter->GetDesc1(&desc);
			if (SUCCEEDED(hr)) {
				if (desc.VendorId == graphicsCardId.vendorId && desc.DeviceId == graphicsCardId.deviceId) {
					if (_TryCreateD3DDevice(adapter)) {
						return true;
					}

					failedIdx = graphicsCardId.idx;
					Logger::Get().Warn("用户指定的显示卡不支持 FL 11");
				} else {
					Logger::Get().Warn("显卡配置已变化");
				}
			}
		}

		// 如果已确认该显卡不支持 FL11，不再重复尝试
		if (failedIdx == -1) {
			// 枚举查找 vendorId 和 deviceId 匹配的显卡
			for (UINT adapterIdx = 0;
				SUCCEEDED(_dxgiFactory->EnumAdapters1(adapterIdx, adapter.put()));
				++adapterIdx
			) {
				if ((int)adapterIdx == graphicsCardId.idx) {
					// 已经检查了 graphicsCardId.idx
					continue;
				}

				DXGI_ADAPTER_DESC1 desc;
				hr = adapter->GetDesc1(&desc);
				if (FAILED(hr)) {
					continue;
				}

				if (desc.VendorId == graphicsCardId.vendorId && desc.DeviceId == graphicsCardId.deviceId) {
					if (_TryCreateD3DDevice(adapter)) {
						return true;
					}

					failedIdx = (int)adapterIdx;
					Logger::Get().Warn("用户指定的显示卡不支持 FL11");
					break;
				}
			}
		}
	}

	// 枚举查找第一个支持 FL11 的显卡
	for (UINT adapterIdx = 0;
		SUCCEEDED(_dxgiFactory->EnumAdapters1(adapterIdx, adapter.put()));
		++adapterIdx
	) {
		if ((int)adapterIdx == failedIdx) {
			// 无需再次尝试
			continue;
		}

		DXGI_ADAPTER_DESC1 desc;
		HRESULT hr = adapter->GetDesc1(&desc);
		if (FAILED(hr) || DirectXHelper::IsWARP(desc)) {
			continue;
		}

		if (_TryCreateD3DDevice(adapter)) {
			return true;
		}
	}

	// 作为最后手段，回落到 CPU 渲染 (WARP)
	// https://docs.microsoft.com/en-us/windows/win32/direct3darticles/directx-warp
	HRESULT hr = _dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&adapter));
	if (FAILED(hr)) {
		Logger::Get().ComError("EnumWarpAdapter 失败", hr);
		return false;
	}

	if (!_TryCreateD3DDevice(adapter)) {
		Logger::Get().ComError("创建 WARP 设备失败", hr);
		return false;
	}

	return true;
}

bool DeviceResources::_TryCreateD3DDevice(const winrt::com_ptr<IDXGIAdapter1>& adapter) noexcept {
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};
	UINT nFeatureLevels = ARRAYSIZE(featureLevels);

	UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
	if (DirectXHelper::IsDebugLayersAvailable()) {
		// 在 DEBUG 配置启用调试层
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	}

	winrt::com_ptr<ID3D11Device> d3dDevice;
	winrt::com_ptr<ID3D11DeviceContext> d3dDC;
	D3D_FEATURE_LEVEL featureLevel;
	HRESULT hr = D3D11CreateDevice(
		adapter.get(),
		D3D_DRIVER_TYPE_UNKNOWN,
		nullptr,
		createDeviceFlags,
		featureLevels,
		nFeatureLevels,
		D3D11_SDK_VERSION,
		d3dDevice.put(),
		&featureLevel,
		d3dDC.put()
	);

	if (FAILED(hr)) {
		Logger::Get().ComError("D3D11CreateDevice 失败", hr);
		return false;
	}

	std::string_view fl;
	switch (featureLevel) {
	case D3D_FEATURE_LEVEL_11_1:
		fl = "11.1";
		break;
	case D3D_FEATURE_LEVEL_11_0:
		fl = "11.0";
		break;
	default:
		fl = "未知";
		break;
	}
	Logger::Get().Info(fmt::format("已创建 D3D 设备\n\t功能级别: {}", fl));

	_d3dDevice = d3dDevice.try_as<ID3D11Device5>();
	if (!_d3dDevice) {
		Logger::Get().Error("获取 ID3D11Device1 失败");
		return false;
	}

	_d3dDC = d3dDC.try_as<ID3D11DeviceContext4>();
	if (!_d3dDC) {
		Logger::Get().Error("获取 ID3D11DeviceContext4 失败");
		return false;
	}
	
	_graphicsAdapter = adapter.try_as<IDXGIAdapter4>();
	if (!_graphicsAdapter) {
		Logger::Get().Error("获取 IDXGIAdapter4 失败");
		return false;
	}

	// 检查半精度浮点支持
	D3D11_FEATURE_DATA_SHADER_MIN_PRECISION_SUPPORT value;
	hr = d3dDevice->CheckFeatureSupport(D3D11_FEATURE_SHADER_MIN_PRECISION_SUPPORT, &value, sizeof(value));
	if (SUCCEEDED(hr)) {
		_isFP16Supported = value.AllOtherShaderStagesMinPrecision & D3D11_SHADER_MIN_PRECISION_16_BIT;
		Logger::Get().Info(StrHelper::Concat("FP16 支持: ", _isFP16Supported ? "是" : "否"));
	} else {
		Logger::Get().ComError("CheckFeatureSupport 失败", hr);
	}

	return true;
}

}
