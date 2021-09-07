#pragma once
#include "pch.h"
#include <variant>


enum class EffectIntermediateTextureFormat {
	B8G8R8A8_UNORM,
	R16G16B16A16_FLOAT
};

struct EffectIntermediateTextureDesc {
	D2D_SIZE_U size;
	EffectIntermediateTextureFormat format;
};

enum class EffectSamplerFilterType {
	Linear,
	Point
};

struct EffectSamplerDesc {
	EffectSamplerFilterType filterType;
};

enum class EffectConstantType {
	Float,
	Int
};

struct EffectConstantDesc {
	std::string name;
	EffectConstantType type;
	std::variant<float, int> defaultValue;
	std::variant<std::monostate, float, int> minValue;
	std::variant<std::monostate, float, int> maxValue;
	bool includeMin = false;
	bool includeMax = false;
};

struct PassDesc {
	std::vector<int> inputs;
	std::vector<int> samplers;
	std::vector<int> constants;
	int output = -1;
};

class Effect {
public:
	bool InitializeFromString(std::string hlsl);

	const std::vector<EffectConstantDesc>& GetConstantDescs() const {
		return _constantDescs;
	}

	bool SetConstant(std::wstring_view var, float value) {
		return true;
	}

	bool SetConstant(std::wstring_view var, int value) {
		return true;
	}

	SIZE CalcOutputSize(SIZE inputSize) const {
		return { lroundf(inputSize.cx * 1.5f), lroundf(inputSize.cy * 1.5f) };
	}

	bool Build(ComPtr<ID3D11Texture2D> input, ComPtr<ID3D11Texture2D> output);

	void Draw();

private:
	class _Pass {
	public:
		bool Initialize(Effect* parent, std::string_view pixelShader);

		bool Build(
			const std::vector<int>& inputs,
			const std::vector<int>& samplers,
			const std::vector<int>& constants,
			ComPtr<ID3D11Texture2D> output,
			std::optional<SIZE> outputSize = {}
		);

		void Draw();

	private:
		Effect* _parent = nullptr;
		ComPtr<ID3D11DeviceContext4> _d3dDC = nullptr;

		ID3D11RenderTargetView* _outputRtv = nullptr;
		
		ComPtr<ID3D11PixelShader> _psShader = nullptr;
		ComPtr<ID3D11VertexShader> _vsShader = nullptr;
		ComPtr<ID3D11InputLayout> _vtxLayout = nullptr;
		ComPtr<ID3D11Buffer> _vtxBuffer = nullptr;

		std::vector<ID3D11ShaderResourceView*> _inputs;
		std::vector<ID3D11SamplerState*> _samplers;
		std::vector<int> _constants;

		D3D11_VIEWPORT _vp{};
	};

	std::vector<ComPtr<ID3D11SamplerState>> _samplers;
	std::vector<ComPtr<ID3D11Texture2D>> _textures;
	std::vector<EffectConstantDesc> _constantDescs;

	std::vector<PassDesc> _passDescs;
	std::vector<_Pass> _passes;
};
