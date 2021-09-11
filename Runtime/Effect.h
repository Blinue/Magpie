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
	EffectConstantType type = EffectConstantType::Float;
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

union Constant32 {
	int intVal;
	float floatVal;
};

struct SimpleVertex {
	XMFLOAT3 Pos;
	XMFLOAT4 TexCoord;
};

class Effect {
public:
	bool InitializeFromString(std::string_view hlsl);
	bool InitializeFromFile(const wchar_t* fileName);
	bool InitializeLanczos();

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

		bool Build(
			const std::vector<int>& inputs,
			const std::vector<int>& samplers,
			ComPtr<ID3D11Texture2D> output
		);

		void Draw();

	private:
		Effect* _parent = nullptr;
		ComPtr<ID3D11DeviceContext4> _d3dDC = nullptr;

		ID3D11RenderTargetView* _outputRtv = nullptr;
		
		ComPtr<ID3D11PixelShader> _psShader = nullptr;

		std::vector<ID3D11ShaderResourceView*> _inputs;
		std::vector<ID3D11SamplerState*> _samplers;

		D3D11_VIEWPORT _vp{};
	};

	ComPtr<ID3D11Device5> _d3dDevice = nullptr;
	ComPtr<ID3D11DeviceContext4> _d3dDC = nullptr;

	std::vector<ComPtr<ID3D11SamplerState>> _samplers;
	std::vector<ComPtr<ID3D11Texture2D>> _textures;

	std::vector<EffectConstantDesc> _constantDescs;
	std::vector<Constant32> _constants;
	ComPtr<ID3D11Buffer> _constantBuffer = nullptr;

	ComPtr<ID3D11VertexShader> _vsShader;

	std::vector<PassDesc> _passDescs;
	std::vector<_Pass> _passes;
};
