#pragma once
#include "EffectDesc.h"

namespace Magpie::Core {

struct EffectOption;

class EffectDrawer {
public:
	EffectDrawer() noexcept = default;
	EffectDrawer(const EffectDrawer&) = delete;
	EffectDrawer(EffectDrawer&&) noexcept = default;

	bool Initialize(
		const EffectDesc& desc,
		const EffectOption& option,
		ID3D11Texture2D* inputTex,
		RECT* outputRect = nullptr,
		RECT* virtualOutputRect = nullptr
	) noexcept;

	ID3D11Texture2D* GetOutputTexture() const noexcept {
		return _textures.empty() ? nullptr : _textures.back().get();
	}

private:
	SmallVector<ID3D11SamplerState*> _samplers;
	SmallVector<winrt::com_ptr<ID3D11Texture2D>> _textures;
	std::vector<SmallVector<ID3D11ShaderResourceView*>> _srvs;
	// 后半部分为空，用于解绑
	std::vector<SmallVector<ID3D11UnorderedAccessView*>> _uavs;
};

}
