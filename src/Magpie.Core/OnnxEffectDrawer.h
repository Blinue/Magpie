#pragma once

namespace Magpie {

class DeviceResources;
class EffectsProfiler;
class InferenceBackendBase;
class BackendDescriptorStore;

#ifdef MAGPIE_ONNX_ENABLED

class OnnxEffectDrawer {
public:
	OnnxEffectDrawer();
	OnnxEffectDrawer(const OnnxEffectDrawer&) = delete;
	OnnxEffectDrawer(OnnxEffectDrawer&&) = default;

	~OnnxEffectDrawer();

	bool Initialize(
		DeviceResources& deviceResources,
		BackendDescriptorStore& descriptorStore,
		ID3D11Texture2D** inOutTexture
	) noexcept;

	void Draw(EffectsProfiler& profiler) const noexcept;


private:
	std::unique_ptr<InferenceBackendBase> _inferenceBackend;

	// 可选的预降采样 / optional pre-downscale before inference
	winrt::com_ptr<ID3D11Texture2D> _downscaledTex;
	winrt::com_ptr<ID3D11ComputeShader> _downscaleShader;
	ID3D11UnorderedAccessView* _downscaledUav = nullptr;
	ID3D11ShaderResourceView* _srcSrv = nullptr;
	ID3D11SamplerState* _sampler = nullptr;
	ID3D11DeviceContext4* _d3dDC = nullptr;
	std::pair<uint32_t, uint32_t> _downscaleDispatch{};
};

#else

// ONNX 只在 x64 上编译；其他平台上这是一个空壳，调用方无需条件编译
// The feature is x64-only. Everywhere else this is an inert stub so callers
// need no conditional compilation: Initialize succeeds and Draw does nothing,
// which is exactly the behaviour when no model is configured.
class OnnxEffectDrawer {
public:
	OnnxEffectDrawer() = default;
	OnnxEffectDrawer(const OnnxEffectDrawer&) = delete;
	OnnxEffectDrawer(OnnxEffectDrawer&&) = default;

	bool Initialize(
		DeviceResources&,
		BackendDescriptorStore&,
		ID3D11Texture2D**
	) noexcept {
		return true;
	}

	void Draw(EffectsProfiler&) const noexcept {}
};

#endif

}
