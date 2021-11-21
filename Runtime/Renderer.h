#pragma once
#include "pch.h"
#include "EffectDrawer.h"
#include "CursorDrawer.h"
#include "FrameRateDrawer.h"
#include <CommonStates.h>
#include "StepTimer.h"
#include "Utils.h"


class Renderer {
public:
	bool Initialize();

	bool InitializeEffectsAndCursor(const std::string& effectsJson);

	void Render();

	bool GetSampler(EffectSamplerFilterType filterType, ID3D11SamplerState** result);

	ComPtr<ID3D11Device1> GetD3DDevice() const{
		return _d3dDevice;
	}

	ComPtr<ID3D11DeviceContext1> GetD3DDC() const {
		return _d3dDC;
	}

	ComPtr<IDXGIDevice1> GetDXGIDevice() const {
		return _dxgiDevice;
	}

	bool GetRenderTargetView(ID3D11Texture2D* texture, ID3D11RenderTargetView** result);

	bool GetShaderResourceView(ID3D11Texture2D* texture, ID3D11ShaderResourceView** result);

	bool SetFillVS();

	bool SetSimpleVS(ID3D11Buffer* simpleVB);

	bool SetCopyPS(ID3D11SamplerState* sampler, ID3D11ShaderResourceView* input);

	bool SetAlphaBlend(bool enable);

	StepTimer& GetTimer() {
		return _timer;
	}

	const StepTimer& GetTimer() const {
		return _timer;
	}

	D3D_FEATURE_LEVEL GetFeatureLevel() const {
		return _featureLevel;
	}

private:
	bool _InitD3D();

	bool _CreateSwapChain();

	bool _CheckSrcState();

	bool _ResolveEffectsJson(const std::string& effectsJson, RECT& destRect);

	void _Render();

	D3D_FEATURE_LEVEL _featureLevel = D3D_FEATURE_LEVEL_10_0;

	ComPtr<IDXGIFactory2> _dxgiFactory;
	ComPtr<IDXGIDevice1> _dxgiDevice;
	ComPtr<IDXGISwapChain2> _dxgiSwapChain;

	ComPtr<ID3D11Device1> _d3dDevice;
	ComPtr<ID3D11DeviceContext1> _d3dDC;

	Utils::ScopedHandle _frameLatencyWaitableObject = NULL;
	bool _waitingForNextFrame = false;

	ComPtr<ID3D11SamplerState> _linearSampler;
	ComPtr<ID3D11SamplerState> _pointSampler;
	ComPtr<ID3D11BlendState> _alphaBlendState;

	ComPtr<ID3D11Texture2D> _effectInput;
	ComPtr<ID3D11Texture2D> _backBuffer;
	std::unordered_map<ID3D11Texture2D*, ComPtr<ID3D11RenderTargetView>> _rtvMap;
	std::unordered_map<ID3D11Texture2D*, ComPtr<ID3D11ShaderResourceView>> _srvMap;

	ComPtr<ID3D11VertexShader> _fillVS;
	ComPtr<ID3D11VertexShader> _simpleVS;
	ComPtr<ID3D11InputLayout> _simpleIL;
	ComPtr<ID3D11PixelShader> _copyPS;
	std::vector<EffectDrawer> _effects;

	CursorDrawer _cursorDrawer;
	FrameRateDrawer _frameRateDrawer;

	StepTimer _timer;
};
