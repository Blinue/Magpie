#pragma once
#include "pch.h"


class EffectDrawer;
class GPUTimer;
class CursorDrawer;
class UIDrawer;
class FSRFilter;


class Renderer {
public:
	Renderer();
	Renderer(const Renderer&) = delete;
	Renderer(Renderer&&) = delete;

	~Renderer();

	bool Initialize(const std::string& effectsJson);

	void Render();

	bool SetFillVS();

	bool SetSimpleVS(ID3D11Buffer* simpleVB);

	bool SetCopyPS(ID3D11SamplerState* sampler, ID3D11ShaderResourceView* input);

	bool SetAlphaBlend(bool enable);

	GPUTimer& GetGPUTimer() {
		return *_gpuTimer;
	}

	CursorDrawer& GetCursorDrawer() noexcept {
		return *_cursorDrawer;
	}

	const RECT& GetOutputRect() const noexcept {
		return _outputRect;
	}

private:
	bool _CheckSrcState();

	bool _ResolveEffectsJson(const std::string& effectsJson);

	RECT _srcWndRect{};
	RECT _outputRect{};

	bool _waitingForNextFrame = false;

	winrt::com_ptr<ID3D11SamplerState> _linearClampSampler;
	winrt::com_ptr<ID3D11SamplerState> _pointClampSampler;
	winrt::com_ptr<ID3D11SamplerState> _linearWrapSampler;
	winrt::com_ptr<ID3D11SamplerState> _pointWrapSampler;
	winrt::com_ptr<ID3D11BlendState> _alphaBlendState;

	winrt::com_ptr<ID3D11Texture2D> _effectInput;

	winrt::com_ptr<ID3D11VertexShader> _fillVS;
	winrt::com_ptr<ID3D11VertexShader> _simpleVS;
	winrt::com_ptr<ID3D11InputLayout> _simpleIL;
	winrt::com_ptr<ID3D11PixelShader> _copyPS;
	std::vector<std::unique_ptr<EffectDrawer>> _effects;

	std::unique_ptr<UIDrawer> _UIDrawer;
	std::unique_ptr<CursorDrawer> _cursorDrawer;
	std::unique_ptr<FSRFilter> _fsrFilter;

	std::unique_ptr<GPUTimer> _gpuTimer;
};
