#pragma once
#include "pch.h"
#include "Effect.h"
#include "CursorRenderer.h"


class Renderer {
public:
	bool Initialize();

	bool InitializeEffects();

	void Render();

	enum class FilterType {
		LINEAR,
		POINT
	};

	bool GetSampler(FilterType filterType, ID3D11SamplerState** result);

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

	ComPtr<ID3D11VertexShader> GetVSShader() const {
		return _vertexShader;
	}

	ComPtr<ID3D11InputLayout> GetInputLayout() const {
		return _inputLayout;
	}

private:
	bool _InitD3D();

	bool _CheckSrcState();

	ComPtr<ID3D11Device5> _d3dDevice;
	ComPtr<IDXGIDevice4> _dxgiDevice;
	ComPtr<IDXGISwapChain4> _dxgiSwapChain;
	ComPtr<ID3D11DeviceContext4> _d3dDC;
	HANDLE _frameLatencyWaitableObject = NULL;
	bool _waitingForNextFrame = false;

	ComPtr<ID3D11SamplerState> _linearSampler;
	ComPtr<ID3D11SamplerState> _pointSampler;

	ComPtr<ID3D11Texture2D> _effectInput;
	ComPtr<ID3D11Texture2D> _effectOutput;
	ComPtr<ID3D11Texture2D> _backBuffer;
	std::unordered_map<ID3D11Texture2D*, ComPtr<ID3D11RenderTargetView>> _rtvMap;
	std::unordered_map<ID3D11Texture2D*, ComPtr<ID3D11ShaderResourceView>> _srvMap;

	ComPtr<ID3D11VertexShader> _vertexShader;
	ComPtr<ID3D11InputLayout> _inputLayout;
	std::vector<Effect> _effects;

	RECT _destRect{};
	CursorRenderer _cursorRenderer;
};
