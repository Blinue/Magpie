#pragma once
#include <parallel_hashmap/phmap.h>

namespace Magpie::Core {

class BackendDescriptorStore {
public:
	BackendDescriptorStore() = default;
	BackendDescriptorStore(const BackendDescriptorStore&) = delete;
	BackendDescriptorStore(BackendDescriptorStore&&) = default;

	void Initialize(ID3D11Device5* d3dDevice) noexcept {
		_d3dDevice = d3dDevice;
	}

	ID3D11ShaderResourceView* GetShaderResourceView(ID3D11Texture2D* texture) noexcept;

	ID3D11UnorderedAccessView* GetUnorderedAccessView(ID3D11Texture2D* texture) noexcept;

	ID3D11UnorderedAccessView* GetUnorderedAccessView(
		ID3D11Buffer* buffer,
		uint32_t numElements,
		DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN
	) noexcept;

private:
	ID3D11Device5* _d3dDevice = nullptr;

	phmap::flat_hash_map<ID3D11Texture2D*, winrt::com_ptr<ID3D11ShaderResourceView>> _srvMap;
	phmap::flat_hash_map<void*, winrt::com_ptr<ID3D11UnorderedAccessView>> _uavMap;
};

}
