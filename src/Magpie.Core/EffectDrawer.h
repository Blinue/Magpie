#pragma once
#include "EffectDesc.h"
#include "SmallVector.h"
#include "EffectHelper.h"

namespace Magpie::Core {

struct EffectOption;
class DeviceResources;

class EffectDrawer {
public:
	EffectDrawer() noexcept = default;
	EffectDrawer(const EffectDrawer&) = delete;
	EffectDrawer(EffectDrawer&&) noexcept = default;

	bool Initialize(
		const EffectDesc& desc,
		const EffectOption& option,
		DeviceResources& deviceResources,
		SIZE scalingWndSize,
		ID3D11Texture2D** inOutTexture
	) noexcept;

	void Draw() const noexcept;

private:
	bool _InitializeConstants(
		const EffectDesc& desc,
		const EffectOption& option,
		DeviceResources& deviceResources,
		SIZE inputSize,
		SIZE outputSize
	) noexcept;

	void _DrawPass(uint32_t i) const noexcept;

	ID3D11DeviceContext* _d3dDC = nullptr;

	SmallVector<ID3D11SamplerState*> _samplers;
	SmallVector<winrt::com_ptr<ID3D11Texture2D>> _textures;
	std::vector<SmallVector<ID3D11ShaderResourceView*>> _srvs;
	// 后半部分为空，用于解绑
	std::vector<SmallVector<ID3D11UnorderedAccessView*>> _uavs;

	SmallVector<EffectHelper::Constant32, 32> _constants;
	winrt::com_ptr<ID3D11Buffer> _constantBuffer;

	SmallVector<winrt::com_ptr<ID3D11ComputeShader>> _shaders;

	SmallVector<std::pair<uint32_t, uint32_t>> _dispatches;
};

}
