#pragma once
#include "pch.h"
#include "EffectDrawer.h"
#include "CursorDrawer.h"
#include "FrameRateDrawer.h"
#include <CommonStates.h>
#include "GPUTimer.h"
#include "Utils.h"


class Renderer {
public:
	bool Initialize();

	bool InitializeEffectsAndCursor(const std::string& effectsJson);

	void Render();

	bool GetSampler(EffectSamplerFilterType filterType, EffectSamplerAddressType addressType, ID3D11SamplerState** result);

	winrt::com_ptr<ID3D11Device1> GetD3DDevice() const{
		return _d3dDevice;
	}

	winrt::com_ptr<ID3D11DeviceContext1> GetD3DDC() const {
		return _d3dDC;
	}

	winrt::com_ptr<IDXGIDevice1> GetDXGIDevice() const {
		return _dxgiDevice;
	}

	winrt::com_ptr<IDXGIFactory2> GetDXGIFactory() const {
		return _dxgiFactory;
	}

	winrt::com_ptr<IDXGIAdapter1> GetGraphicsAdapter() const {
		return _graphicsAdapter;
	}

	bool GetRenderTargetView(ID3D11Texture2D* texture, ID3D11RenderTargetView** result);

	bool GetShaderResourceView(ID3D11Texture2D* texture, ID3D11ShaderResourceView** result);

	bool SetFillVS();

	bool SetSimpleVS(ID3D11Buffer* simpleVB);

	bool SetCopyPS(ID3D11SamplerState* sampler, ID3D11ShaderResourceView* input);

	bool SetAlphaBlend(bool enable);

	GPUTimer& GetTimer() {
		return _gpuTimer;
	}

	const GPUTimer& GetTimer() const {
		return _gpuTimer;
	}

	D3D_FEATURE_LEVEL GetFeatureLevel() const {
		return _featureLevel;
	}

	bool CompileShader(bool isVS, std::string_view hlsl, const char* entryPoint,
		ID3DBlob** blob, const char* sourceName = nullptr, ID3DInclude* include = nullptr);

	// 测试 D3D 调试层是否可用
	static bool IsDebugLayersAvailable();

private:
	bool _InitD3D();

	bool _CreateSwapChain();

	bool _CheckSrcState();

	bool _ResolveEffectsJson(const std::string& effectsJson, RECT& destRect);

	RECT _srcWndRect{};

	D3D_FEATURE_LEVEL _featureLevel = D3D_FEATURE_LEVEL_10_0;
	bool _supportTearing = false;

	winrt::com_ptr<IDXGIFactory4> _dxgiFactory;
	winrt::com_ptr<IDXGIDevice1> _dxgiDevice;
	winrt::com_ptr<IDXGISwapChain2> _dxgiSwapChain;
	winrt::com_ptr<IDXGIAdapter1> _graphicsAdapter;
	winrt::com_ptr<ID3D11Device1> _d3dDevice;
	winrt::com_ptr<ID3D11DeviceContext1> _d3dDC;

	Utils::ScopedHandle _frameLatencyWaitableObject = NULL;
	bool _waitingForNextFrame = false;

	winrt::com_ptr<ID3D11SamplerState> _linearClampSampler;
	winrt::com_ptr<ID3D11SamplerState> _pointClampSampler;
	winrt::com_ptr<ID3D11SamplerState> _linearWrapSampler;
	winrt::com_ptr<ID3D11SamplerState> _pointWrapSampler;
	winrt::com_ptr<ID3D11BlendState> _alphaBlendState;

	winrt::com_ptr<ID3D11Texture2D> _effectInput;
	winrt::com_ptr<ID3D11Texture2D> _backBuffer;
	std::unordered_map<ID3D11Texture2D*, winrt::com_ptr<ID3D11RenderTargetView>> _rtvMap;
	std::unordered_map<ID3D11Texture2D*, winrt::com_ptr<ID3D11ShaderResourceView>> _srvMap;

	winrt::com_ptr<ID3D11VertexShader> _fillVS;
	winrt::com_ptr<ID3D11VertexShader> _simpleVS;
	winrt::com_ptr<ID3D11InputLayout> _simpleIL;
	winrt::com_ptr<ID3D11PixelShader> _copyPS;
	std::vector<EffectDrawer> _effects;

	CursorDrawer _cursorDrawer;
	FrameRateDrawer _frameRateDrawer;

	GPUTimer _gpuTimer;
};
