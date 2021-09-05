#pragma once
#include "pch.h"
#include "Effect.h"


class Renderer {
public:
	bool Initialize();

	bool InitializeEffects(ComPtr<ID3D11Texture2D> input);

	void Render();

	enum class FilterType {
		LINEAR,
		POINT
	};

	ComPtr<ID3D11SamplerState> GetSampler(FilterType filterType);

	ComPtr<ID3D11Device5> GetD3DDevice() const{
		return _d3dDevice;
	}

	ComPtr<ID3D11DeviceContext4> GetD3DDC() const {
		return _d3dDC;
	}

	ComPtr<IDXGIDevice4> GetDXGIDevice() const {
		return _dxgiDevice;
	}

	HRESULT GetRenderTargetView(ID3D11Texture2D* texture, ID3D11RenderTargetView** result);

	HRESULT GetShaderResourceView(ID3D11Texture2D* texture, ID3D11ShaderResourceView** result);

private:
	bool _InitD3D();

	ComPtr<ID3D11Device5> _d3dDevice = nullptr;
	ComPtr<IDXGIDevice4> _dxgiDevice = nullptr;
	ComPtr<IDXGISwapChain1> _dxgiSwapChain = nullptr;
	ComPtr<ID3D11DeviceContext4> _d3dDC = nullptr;

	ComPtr<ID3D11SamplerState> _linearSampler = nullptr;
	ComPtr<ID3D11SamplerState> _pointSampler = nullptr;

	ComPtr<ID3D11Texture2D> _backBuffer = nullptr;
	std::unordered_map<ID3D11Texture2D*, ComPtr<ID3D11RenderTargetView>> _rtvMap;
	std::unordered_map<ID3D11Texture2D*, ComPtr<ID3D11ShaderResourceView>> _srvMap;

	std::vector<Effect> _effects;
};
