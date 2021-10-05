#pragma once
#include "pch.h"
#include "EffectDesc.h"
#include <optional>


union Constant32 {
	int intVal;
	float floatVal;
};


class Effect {
public:
	bool InitializeFromString(std::string_view hlsl);
	bool InitializeFromFile(const wchar_t* fileName);
	bool InitializeFsr();

	const std::vector<EffectConstantDesc>& GetConstantDescs() const {
		return _constantDescs;
	}

	bool SetConstant(int index, float value);

	bool SetConstant(int index, int value);

	SIZE CalcOutputSize(SIZE inputSize) const;

	bool CanSetOutputSize() const;

	void SetOutputSize(SIZE value);

	bool Build(ComPtr<ID3D11Texture2D> input, ComPtr<ID3D11Texture2D> output);

	void Draw();

private:
	class _Pass {
	public:
		bool Initialize(Effect* parent, const std::string& pixelShader);

		bool Build(const std::vector<int>& inputs, int output, std::optional<SIZE> outputSize);

		void Draw();

	private:
		Effect* _parent = nullptr;
		ComPtr<ID3D11DeviceContext> _d3dDC;

		ID3D11RenderTargetView* _outputRtv = nullptr;
		
		ComPtr<ID3D11PixelShader> _pixelShader;

		std::vector<ID3D11ShaderResourceView*> _inputs;
		std::vector<ID3D11SamplerState*> _samplers;

		ComPtr<ID3D11Buffer> _vtxBuffer;
		D3D11_VIEWPORT _vp{};
	};

	ComPtr<ID3D11Device> _d3dDevice;
	ComPtr<ID3D11DeviceContext> _d3dDC;

	std::vector<ID3D11SamplerState*> _samplers;
	std::vector<ComPtr<ID3D11Texture2D>> _textures;

	std::vector<EffectConstantDesc> _constantDescs;
	std::vector<Constant32> _constants;
	ComPtr<ID3D11Buffer> _constantBuffer;

	ComPtr<ID3D11VertexShader> _vertexShader;

	std::optional<SIZE> _outputSize;

	std::vector<PassDesc> _passDescs;
	std::vector<_Pass> _passes;
};
