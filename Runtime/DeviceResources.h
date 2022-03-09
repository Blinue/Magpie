#pragma once
#include "pch.h"
#include "Utils.h"


class DeviceResources {
public:
	DeviceResources() = default;
	DeviceResources(const DeviceResources&) = delete;
	DeviceResources(DeviceResources&&) = delete;

	bool Initialize();

	static bool IsDebugLayersAvailable();

	winrt::com_ptr<ID3D11Texture2D> CreateTexture2D(
		DXGI_FORMAT format,
		UINT width,
		UINT height,
		UINT bindFlags,
		D3D11_USAGE usage = D3D11_USAGE_DEFAULT,
		UINT miscFlags = 0,
		const D3D11_SUBRESOURCE_DATA* pInitialData = nullptr
	);

	bool GetSampler(D3D11_FILTER filterMode, D3D11_TEXTURE_ADDRESS_MODE addressMode, ID3D11SamplerState** result);

	bool GetRenderTargetView(ID3D11Texture2D* texture, ID3D11RenderTargetView** result);

	bool GetShaderResourceView(ID3D11Texture2D* texture, ID3D11ShaderResourceView** result);

	bool GetUnorderedAccessView(ID3D11Texture2D* texture, ID3D11UnorderedAccessView** result);

	bool CompileShader(std::string_view hlsl, const char* entryPoint,
		ID3DBlob** blob, const char* sourceName = nullptr, ID3DInclude* include = nullptr);

	ID3D11Device3* GetD3DDevice() const noexcept { return _d3dDevice.get(); }
	D3D_FEATURE_LEVEL GetFeatureLevel() const noexcept { return _featureLevel; }
	ID3D11DeviceContext3* GetD3DDC() const noexcept { return _d3dDC.get(); }
	IDXGISwapChain4* GetSwapChain() const noexcept { return _swapChain.get(); };
	ID3D11Texture2D* GetBackBuffer() const noexcept { return _backBuffer.get(); }
	IDXGIFactory5* GetDXGIFactory() const noexcept { return _dxgiFactory.get(); }
	IDXGIDevice4* GetDXGIDevice() const noexcept { return _dxgiDevice.get(); }
	IDXGIAdapter3* GetGraphicsAdapter() const noexcept { return _graphicsAdapter.get(); }

	void BeginFrame();

	void EndFrame();

private:
	bool _CreateSwapChain();

	winrt::com_ptr<IDXGIFactory5> _dxgiFactory;
	winrt::com_ptr<IDXGIDevice4> _dxgiDevice;
	winrt::com_ptr<IDXGISwapChain4> _swapChain;
	winrt::com_ptr<IDXGIAdapter3> _graphicsAdapter;
	winrt::com_ptr<ID3D11Device3> _d3dDevice;
	winrt::com_ptr<ID3D11DeviceContext3> _d3dDC;

	Utils::ScopedHandle _frameLatencyWaitableObject;
	bool _supportTearing = false;
	D3D_FEATURE_LEVEL _featureLevel = D3D_FEATURE_LEVEL_10_0;

	winrt::com_ptr<ID3D11Texture2D> _backBuffer;

	std::unordered_map<ID3D11Texture2D*, winrt::com_ptr<ID3D11RenderTargetView>> _rtvMap;
	std::unordered_map<ID3D11Texture2D*, winrt::com_ptr<ID3D11ShaderResourceView>> _srvMap;
	std::unordered_map<ID3D11Texture2D*, winrt::com_ptr<ID3D11UnorderedAccessView>> _uavMap;

	std::unordered_map<
		std::pair<D3D11_FILTER, D3D11_TEXTURE_ADDRESS_MODE>,
		winrt::com_ptr<ID3D11SamplerState>
	> _samMap;
};
