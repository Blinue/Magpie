#include "pch.h"
#include "DeviceResources.h"
#include "ScalingOptions.h"
#include "Logger.h"
#include "StrUtils.h"
#include "DirectXHelper.h"

namespace Magpie::Core {

bool DeviceResources::Initialize(const ScalingOptions& options) noexcept {
#ifdef _DEBUG
	UINT flag = DXGI_CREATE_FACTORY_DEBUG;
#else
	UINT flag = 0;
#endif // _DEBUG

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

	_isSupportTearing = supportTearing;
	Logger::Get().Info(fmt::format("可变刷新率支持：{}", supportTearing ? "是" : "否"));

	if (!_ObtainAdapterAndDevice(options.graphicsCard)) {
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

	D3D11_SAMPLER_DESC desc{};
	desc.Filter = filterMode;
	desc.AddressU = addressMode;
	desc.AddressV = addressMode;
	desc.AddressW = addressMode;
	desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	desc.MinLOD = 0;
	desc.MaxLOD = 0;
	HRESULT hr = _d3dDevice->CreateSamplerState(&desc, sam.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("创建 ID3D11SamplerState 出错", hr);
		return nullptr;
	}

	return _samMap.emplace(key, std::move(sam)).first->second.get();
}

ID3D11RenderTargetView* DeviceResources::GetRenderTargetView(ID3D11Texture2D* texture) noexcept {
	auto it = _rtvMap.find(texture);
	if (it != _rtvMap.end()) {
		return it->second.get();
	}

	winrt::com_ptr<ID3D11RenderTargetView> rtv;
	HRESULT hr = _d3dDevice->CreateRenderTargetView(texture, nullptr, rtv.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateRenderTargetView 失败", hr);
		return nullptr;
	}

	return _rtvMap.emplace(texture, std::move(rtv)).first->second.get();
}

ID3D11ShaderResourceView* DeviceResources::GetShaderResourceView(ID3D11Texture2D* texture) noexcept {
	auto it = _srvMap.find(texture);
	if (it != _srvMap.end()) {
		return it->second.get();
	}

	winrt::com_ptr<ID3D11ShaderResourceView> srv;
	HRESULT hr = _d3dDevice->CreateShaderResourceView(texture, nullptr, srv.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateShaderResourceView 失败", hr);
		return nullptr;
	}

	return _srvMap.emplace(texture, std::move(srv)).first->second.get();
}

ID3D11UnorderedAccessView* DeviceResources::GetUnorderedAccessView(ID3D11Texture2D* texture) noexcept {
	auto it = _uavMap.find(texture);
	if (it != _uavMap.end()) {
		return it->second.get();
	}

	winrt::com_ptr<ID3D11UnorderedAccessView> uav;

	D3D11_UNORDERED_ACCESS_VIEW_DESC desc{};
	desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	desc.Texture2D.MipSlice = 0;

	HRESULT hr = _d3dDevice->CreateUnorderedAccessView(texture, &desc, uav.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateUnorderedAccessView 失败", hr);
		return nullptr;
	}

	return _uavMap.emplace(texture, std::move(uav)).first->second.get();
}

static void LogAdapter(const DXGI_ADAPTER_DESC1& adapterDesc) noexcept {
	Logger::Get().Info(fmt::format("当前图形适配器：\n\tVendorId：{:#x}\n\tDeviceId：{:#x}\n\t描述：{}",
		adapterDesc.VendorId, adapterDesc.DeviceId, StrUtils::UTF16ToUTF8(adapterDesc.Description)));
}

bool DeviceResources::_ObtainAdapterAndDevice(int adapterIdx) noexcept {
	winrt::com_ptr<IDXGIAdapter1> adapter;

	if (adapterIdx >= 0) {
		HRESULT hr = _dxgiFactory->EnumAdapters1(adapterIdx, adapter.put());
		if (SUCCEEDED(hr)) {
			DXGI_ADAPTER_DESC1 desc;
			hr = adapter->GetDesc1(&desc);
			if (SUCCEEDED(hr)) {
				if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
					Logger::Get().Warn("用户指定的显示卡为 WARP，已忽略");
				} else if (_TryCreateD3DDevice(adapter)) {
					LogAdapter(desc);
					return true;
				} else {
					Logger::Get().Warn("用户指定的显示卡不支持 FL 11");
				}
			} else {
				Logger::Get().Error("GetDesc1 失败");
			}
		} else {
			Logger::Get().Warn("未找到用户指定的显示卡");
		}
	}

	// 枚举查找第一个支持 D3D11 的图形适配器
	for (UINT adapterIndex = 0;
		SUCCEEDED(_dxgiFactory->EnumAdapters1(adapterIndex, adapter.put()));
		++adapterIndex
	) {
		DXGI_ADAPTER_DESC1 desc;
		HRESULT hr = adapter->GetDesc1(&desc);
		if (FAILED(hr)) {
			continue;
		}

		// 忽略 WARP
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
			continue;
		}

		if (_TryCreateD3DDevice(adapter)) {
			LogAdapter(desc);
			return true;
		}
	}

	// 作为最后手段，回落到 Basic Render Driver Adapter（WARP）
	// https://docs.microsoft.com/en-us/windows/win32/direct3darticles/directx-warp
	HRESULT hr = _dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&adapter));
	if (FAILED(hr)) {
		Logger::Get().ComError("创建 WARP 设备失败", hr);
		return false;
	}

	Logger::Get().Info("已创建 WARP 设备");
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
	Logger::Get().Info(fmt::format("已创建 D3D Device\n\t功能级别：{}", fl));

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
		Logger::Get().ComError("获取 IDXGIAdapter4 失败", hr);
		return false;
	}

	return true;
}

}
