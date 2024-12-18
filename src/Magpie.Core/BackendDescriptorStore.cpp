#include "pch.h"
#include "BackendDescriptorStore.h"
#include "Logger.h"

namespace Magpie {

ID3D11ShaderResourceView* BackendDescriptorStore::GetShaderResourceView(ID3D11Texture2D* texture) noexcept {
	if (auto it = _srvMap.find(texture); it != _srvMap.end()) {
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

ID3D11UnorderedAccessView* BackendDescriptorStore::GetUnorderedAccessView(ID3D11Texture2D* texture) noexcept {
	if (auto it = _uavMap.find(texture); it != _uavMap.end()) {
		return it->second.get();
	}

	winrt::com_ptr<ID3D11UnorderedAccessView> uav;

	D3D11_UNORDERED_ACCESS_VIEW_DESC desc{
		.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D
	};

	HRESULT hr = _d3dDevice->CreateUnorderedAccessView(texture, &desc, uav.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateUnorderedAccessView 失败", hr);
		return nullptr;
	}

	return _uavMap.emplace(texture, std::move(uav)).first->second.get();
}

ID3D11UnorderedAccessView* BackendDescriptorStore::GetUnorderedAccessView(ID3D11Buffer* buffer, uint32_t numElements, DXGI_FORMAT format) noexcept {
	if (auto it = _uavMap.find(buffer); it != _uavMap.end()) {
		return it->second.get();
	}

	winrt::com_ptr<ID3D11UnorderedAccessView> uav;

	D3D11_UNORDERED_ACCESS_VIEW_DESC desc{
		.Format = format,
		.ViewDimension = D3D11_UAV_DIMENSION_BUFFER,
		.Buffer{
			.NumElements = numElements
		}
	};

	HRESULT hr = _d3dDevice->CreateUnorderedAccessView(buffer, &desc, uav.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateUnorderedAccessView 失败", hr);
		return nullptr;
	}

	return _uavMap.emplace(buffer, std::move(uav)).first->second.get();
}

}
