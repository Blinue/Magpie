#pragma once
#include "pch.h"
#include "EffectDesc.h"
#include <optional>


union Constant32 {
	int intVal;
	float floatVal;
};


class EffectDrawer {
public:
	bool Initialize(const wchar_t* fileName);

	const std::vector<EffectConstantDesc>& GetConstantDescs() const {
		return _effectDesc.constants;
	}

	bool SetConstant(int index, float value);

	bool SetConstant(int index, int value);

	bool CalcOutputSize(SIZE inputSize, SIZE& outputSize) const;

	bool CanSetOutputSize() const;

	void SetOutputSize(SIZE value);

	bool Build(ComPtr<ID3D11Texture2D> input, ComPtr<ID3D11Texture2D> output);

	void Draw();

private:
	class _Pass {
	public:
		bool Initialize(EffectDrawer* parent, ComPtr<ID3DBlob> shaderBlob);

		bool Build(const std::vector<UINT>& inputs, UINT output, std::optional<SIZE> outputSize);

		void Draw();

	private:
		EffectDrawer* _parent = nullptr;
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

	std::vector<Constant32> _constants;
	ComPtr<ID3D11Buffer> _constantBuffer;

	ComPtr<ID3D11VertexShader> _vertexShader;

	std::optional<SIZE> _outputSize;

	EffectDesc _effectDesc{};
	std::vector<_Pass> _passes;
};
