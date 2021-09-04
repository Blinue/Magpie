#pragma once
#include "pch.h"
#include "Env.h"
#include <directxcolors.h>
#include <variant>

using namespace DirectX;


struct SimpleVertex {
	XMFLOAT3 Pos;
	XMFLOAT4 TexCoord;
};

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
	Int,
	Bool
};

struct EffectConstantDesc {
	std::string name;
	EffectConstantType type;
	std::variant<float, int, bool> defaultValue;
	std::variant<std::monostate, float, int> minValue;
	std::variant<std::monostate, float, int> maxValue;
	bool includeMin = false;
	bool includeMax = false;
};



class Effect {
public:
	Effect() {};

	Effect(D2D_SIZE_U inputSize, ID3D11RenderTargetView* output, ID3D11SamplerState* linearSampler, D2D1_VECTOR_2F scale);

	bool Initialize(D2D_SIZE_U inputSize) {

	}

	std::vector<EffectIntermediateTextureDesc> GetIntermediateTextureDescs() const {

	}

	std::vector<EffectSamplerDesc> GetSamplerDescs() const {

	}

	std::vector<EffectConstantDesc> GetConstantDescs() const {

	}

	void Draw();

	void BindResources(
		ComPtr<ID3D11Texture2D> input,
		std::vector<ComPtr<ID3D11Texture2D>> intermediateTextures,
		std::vector<ComPtr<ID3D11SamplerState>> samplers
	) {

	}

	void SetConstant(std::wstring_view var, float value) {

	}

	D2D_SIZE_U GetOutputSize() const {

	}

	void SetOutput(ComPtr<ID3D11Texture2D> output) {

	}

private:
	HRESULT _CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);


	ID3D11Device3* _d3dDevice = nullptr;
	ID3D11DeviceContext4* _d3dDC = nullptr;
	ID3D11SamplerState* _linearSampler = nullptr;
	ComPtr<ID3D11VertexShader> _vsShader = nullptr;
	ComPtr<ID3D11PixelShader> _psShader = nullptr;
	ComPtr<ID3D11InputLayout> _vtxLayout = nullptr;
	ComPtr<ID3D11Buffer> _vtxBuffer = nullptr;

	ID3D11RenderTargetView* _output;
	D3D11_VIEWPORT _vp{};
};