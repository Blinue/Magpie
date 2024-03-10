#pragma once

namespace Magpie::Core {

class DeviceResources;
class EffectsProfiler;
class InferenceBackendBase;
class BackendDescriptorStore;

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
};

}
