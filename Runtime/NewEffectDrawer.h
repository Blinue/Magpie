#pragma once
#include "pch.h"
#include "EffectDesc.h"


class NewEffectDrawer {
public:
	NewEffectDrawer() = default;
	NewEffectDrawer(const NewEffectDrawer&) = default;
	NewEffectDrawer(NewEffectDrawer&&) = default;

	bool Initialize(
		const EffectDesc& desc,
		const EffectParams& params,
		ID3D11Texture2D* inputTex,
		ID3D11Texture2D** outputTex,
		RECT* outputRect = nullptr
	);

	void Draw();
private:
	EffectDesc _desc;

	std::vector<ID3D11SamplerState*> _samplers;
	std::vector<winrt::com_ptr<ID3D11Texture2D>> _textures;
	std::vector<std::vector<ID3D11ShaderResourceView*>> _rtvs;
	std::vector<std::vector<ID3D11UnorderedAccessView*>> _uavs;

	std::vector<EffectConstant32> _constants;
	winrt::com_ptr<ID3D11Buffer> _constantBuffer;

	std::vector<winrt::com_ptr<ID3D11ComputeShader>> _shaders;
};
