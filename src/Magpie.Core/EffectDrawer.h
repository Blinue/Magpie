#pragma once
#include "EffectDesc.h"
#include "SmallVector.h"
#include "EffectHelper.h"

namespace Magpie {

struct EffectOption;
class DeviceResources;
class BackendDescriptorStore;
class EffectsProfiler;

class EffectDrawer {
public:
	EffectDrawer() = default;
	EffectDrawer(const EffectDrawer&) = delete;
	EffectDrawer(EffectDrawer&&) = default;

	~EffectDrawer();

	bool Initialize(
		const EffectDesc& desc,
		const EffectOption& option,
		bool treatFitAsFill,
		DeviceResources& deviceResources,
		BackendDescriptorStore& descriptorStore,
		ID3D11Texture2D** inOutTexture
	) noexcept;

	void Draw(EffectsProfiler& profiler) const noexcept;

	void DrawForScreenshot(const EffectDesc& desc, int passIdx) const noexcept;

	bool ResizeTextures(
		const EffectDesc& desc,
		const EffectOption& option,
		bool treatFitAsFill,
		DeviceResources& deviceResources,
		ID3D11Texture2D** inOutTexture
	) noexcept;

	ID3D11Texture2D* GetOutputTexture() const noexcept {
		return _textures[1].get();
	}

	ID3D11Texture2D* GetTexture(uint32_t idx) const noexcept {
		return _textures[idx].get();
	}

private:
	bool _UpdatePassResources(const EffectDesc& desc) noexcept;

	bool _UpdateConstants(
		const EffectDesc& desc,
		const EffectOption& option,
		DeviceResources& deviceResources,
		SIZE inputSize,
		SIZE outputSize
	) noexcept;

	void _PrepareForDraw() const noexcept;

	void _DrawPass(uint32_t i) const noexcept;

	ID3D11DeviceContext* _d3dDC = nullptr;
	BackendDescriptorStore* _descriptorStore = nullptr;

	SmallVector<ID3D11SamplerState*> _samplers;
	SmallVector<winrt::com_ptr<ID3D11Texture2D>> _textures;
	std::vector<SmallVector<ID3D11ShaderResourceView*>> _srvs;
	// 后半部分为空，用于解绑
	std::vector<SmallVector<ID3D11UnorderedAccessView*>> _uavs;

	winrt::com_ptr<ID3D11Buffer> _constantBuffer;

	SmallVector<winrt::com_ptr<ID3D11ComputeShader>> _shaders;

	SmallVector<std::pair<uint32_t, uint32_t>> _dispatches;
};

}
