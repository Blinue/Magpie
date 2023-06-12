#pragma once
#include <parallel_hashmap/phmap.h>

namespace Magpie::Core {

struct ScalingOptions;

class DeviceResources {
public:
	DeviceResources() noexcept = default;
	DeviceResources(const DeviceResources&) = delete;
	DeviceResources(DeviceResources&&) noexcept = default;

	bool Initialize(const ScalingOptions& options) noexcept;

	IDXGIFactory7* GetDXGIFactory() const noexcept { return _dxgiFactory.get(); }
	ID3D11Device5* GetD3DDevice() const noexcept { return _d3dDevice.get(); }
	ID3D11DeviceContext4* GetD3DDC() const noexcept { return _d3dDC.get(); }
	IDXGIAdapter4* GetGraphicsAdapter() const noexcept { return _graphicsAdapter.get(); }

	bool IsSupportTearing() const noexcept {
		return _isSupportTearing;
	}

	ID3D11SamplerState* GetSampler(D3D11_FILTER filterMode, D3D11_TEXTURE_ADDRESS_MODE addressMode) noexcept;

	ID3D11RenderTargetView* GetRenderTargetView(ID3D11Texture2D* texture) noexcept;

	ID3D11ShaderResourceView* GetShaderResourceView(ID3D11Texture2D* texture) noexcept;

	ID3D11UnorderedAccessView* GetUnorderedAccessView(ID3D11Texture2D* texture) noexcept;

private:
	bool _ObtainAdapterAndDevice(int adapterIdx) noexcept;
	bool _TryCreateD3DDevice(const winrt::com_ptr<IDXGIAdapter1>& adapter) noexcept;

	winrt::com_ptr<IDXGIFactory7> _dxgiFactory;
	winrt::com_ptr<IDXGIAdapter4> _graphicsAdapter;
	winrt::com_ptr<ID3D11Device5> _d3dDevice;
	winrt::com_ptr<ID3D11DeviceContext4> _d3dDC;

	phmap::flat_hash_map<ID3D11Texture2D*, winrt::com_ptr<ID3D11RenderTargetView>> _rtvMap;
	phmap::flat_hash_map<ID3D11Texture2D*, winrt::com_ptr<ID3D11ShaderResourceView>> _srvMap;
	phmap::flat_hash_map<ID3D11Texture2D*, winrt::com_ptr<ID3D11UnorderedAccessView>> _uavMap;

	phmap::flat_hash_map<
		std::pair<D3D11_FILTER, D3D11_TEXTURE_ADDRESS_MODE>,
		winrt::com_ptr<ID3D11SamplerState>
	> _samMap;

	bool _isSupportTearing = false;
};

}
