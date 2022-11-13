#pragma once
#include "pch.h"
#include "EffectDesc.h"
#include "MagOptions.h"
#include "SmallVector.h"


namespace Magpie::Core {

union EffectConstant32 {
	float floatVal;
	uint32_t uintVal;
	int intVal;
};

class EffectDrawer {
public:
	EffectDrawer() = default;
	EffectDrawer(const EffectDrawer&) = delete;
	EffectDrawer(EffectDrawer&&) = default;

	bool Initialize(
		const EffectDesc& desc,
		const EffectOption& option,
		ID3D11Texture2D* inputTex,
		RECT* outputRect = nullptr,
		RECT* virtualOutputRect = nullptr
	);

	void Draw(UINT& idx, bool noUpdate = false);

	bool IsUseDynamic() const noexcept {
		return _desc.flags & EffectFlags::UseDynamic;
	}

	const EffectDesc& GetDesc() const noexcept {
		return _desc;
	}

	ID3D11Texture2D* GetOutputTexture() const noexcept {
		return _textures.empty() ? nullptr : _textures.back().get();
	}

private:
	void _DrawPass(UINT i);

	EffectDesc _desc;

	SmallVector<ID3D11SamplerState*> _samplers;
	SmallVector<winrt::com_ptr<ID3D11Texture2D>> _textures;
	std::vector<SmallVector<ID3D11ShaderResourceView*>> _srvs;
	// 后半部分为空，用于解绑
	std::vector<SmallVector<ID3D11UnorderedAccessView*>> _uavs;

	SmallVector<EffectConstant32, 32> _constants;
	winrt::com_ptr<ID3D11Buffer> _constantBuffer;

	SmallVector<winrt::com_ptr<ID3D11ComputeShader>> _shaders;

	SmallVector<std::pair<UINT, UINT>> _dispatches;
};

}
