#pragma once
#include "pch.h"
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

	bool Initialize(ComPtr<ID3D11Texture2D> input);

	std::vector<EffectConstantDesc> GetConstantDescs() const {
		return {};
	}

	void Draw();

	void SetConstant(std::wstring_view var, float value) {

	}

	SIZE GetOutputSize() const {
		return _inputSize;
	}

	bool SetOutput(ComPtr<ID3D11Texture2D> output);

private:
	HRESULT _CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);

	ComPtr<ID3D11Device5> _d3dDevice = nullptr;
	ComPtr<ID3D11DeviceContext4> _d3dDC = nullptr;
	ComPtr<ID3D11VertexShader> _vsShader = nullptr;
	ComPtr<ID3D11PixelShader> _psShader = nullptr;
	ComPtr<ID3D11InputLayout> _vtxLayout = nullptr;
	ComPtr<ID3D11Buffer> _vtxBuffer = nullptr;

	ID3D11SamplerState* _sampler = nullptr;

	ID3D11ShaderResourceView* _inputSrv = nullptr;
	ID3D11RenderTargetView* _outputRtv = nullptr;
	D3D11_VIEWPORT _vp{};

	SIZE _inputSize{};
};