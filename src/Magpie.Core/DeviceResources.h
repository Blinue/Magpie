#pragma once
#include <parallel_hashmap/phmap.h>
#include "ScalingOptions.h"

namespace Magpie {

class DeviceResources {
public:
	DeviceResources() = default;
	DeviceResources(const DeviceResources&) = delete;
	DeviceResources(DeviceResources&&) = default;

	bool Initialize() noexcept;

	IDXGIFactory7* GetDXGIFactory() const noexcept { return _dxgiFactory.get(); }
	ID3D11Device5* GetD3DDevice() const noexcept { return _d3dDevice.get(); }
	ID3D11DeviceContext4* GetD3DDC() const noexcept { return _d3dDC.get(); }
	IDXGIAdapter4* GetGraphicsAdapter() const noexcept { return _graphicsAdapter.get(); }

	bool IsTearingSupported() const noexcept {
		return _isTearingSupported;
	}

	ID3D11SamplerState* GetSampler(D3D11_FILTER filterMode, D3D11_TEXTURE_ADDRESS_MODE addressMode) noexcept;

private:
	bool _ObtainAdapterAndDevice(GraphicsCardId graphicsCardId) noexcept;
	bool _TryCreateD3DDevice(const winrt::com_ptr<IDXGIAdapter1>& adapter) noexcept;

	winrt::com_ptr<IDXGIFactory7> _dxgiFactory;
	winrt::com_ptr<IDXGIAdapter4> _graphicsAdapter;
	winrt::com_ptr<ID3D11Device5> _d3dDevice;
	winrt::com_ptr<ID3D11DeviceContext4> _d3dDC;

	phmap::flat_hash_map<
		std::pair<D3D11_FILTER, D3D11_TEXTURE_ADDRESS_MODE>,
		winrt::com_ptr<ID3D11SamplerState>
	> _samMap;

	bool _isTearingSupported = false;
	bool _isFP16Supported = false;
};

}
