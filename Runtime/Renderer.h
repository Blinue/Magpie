#pragma once
#include "pch.h"
#include "Effect.h"
#include "CursorRenderer.h"
#include "FrameRateRenderer.h"


class Renderer {
public:
	bool Initialize();

	bool InitializeEffectsAndCursor();

	void Render();

	enum class FilterType {
		LINEAR,
		POINT
	};

	bool GetSampler(FilterType filterType, ID3D11SamplerState** result);

	ComPtr<ID3D11Device3> GetD3DDevice() const{
		return _d3dDevice;
	}

	ComPtr<ID3D11DeviceContext3> GetD3DDC() const {
		return _d3dDC;
	}

	ComPtr<IDXGIDevice3> GetDXGIDevice() const {
		return _dxgiDevice;
	}

	HRESULT GetRenderTargetView(ID3D11Texture2D* texture, ID3D11RenderTargetView** result);

	HRESULT GetShaderResourceView(ID3D11Texture2D* texture, ID3D11ShaderResourceView** result);

	void SetFillVS();

	void SetCopyPS(ID3D11SamplerState* sampler, ID3D11ShaderResourceView* input);

private:
	bool _InitD3D();

	bool _CheckSrcState();

	ComPtr<ID3D11Device3> _d3dDevice;
	ComPtr<IDXGIDevice4> _dxgiDevice;
	ComPtr<IDXGISwapChain4> _dxgiSwapChain;
	ComPtr<ID3D11DeviceContext3> _d3dDC;
	HANDLE _frameLatencyWaitableObject = NULL;
	bool _waitingForNextFrame = false;

	ComPtr<ID3D11SamplerState> _linearSampler;
	ComPtr<ID3D11SamplerState> _pointSampler;

	ComPtr<ID3D11Texture2D> _effectInput;
	ComPtr<ID3D11Texture2D> _effectOutput;
	ComPtr<ID3D11Texture2D> _backBuffer;
	std::unordered_map<ID3D11Texture2D*, ComPtr<ID3D11RenderTargetView>> _rtvMap;
	std::unordered_map<ID3D11Texture2D*, ComPtr<ID3D11ShaderResourceView>> _srvMap;

	ComPtr<ID3D11VertexShader> _fillVS;
	ComPtr<ID3D11PixelShader> _copyPS;
	std::vector<Effect> _effects;

	RECT _destRect{};
	CursorRenderer _cursorRenderer;
	FrameRateRenderer _frameRateRenderer;
};
