#pragma once

namespace Magpie::Core {

class DeviceResources;
class BackendDescriptorStore;

class TensorRTInferenceEngine {
public:
	TensorRTInferenceEngine() = default;
	TensorRTInferenceEngine(const TensorRTInferenceEngine&) = delete;
	TensorRTInferenceEngine(TensorRTInferenceEngine&&) = default;

	bool Initialize(
		const wchar_t* modelPath,
		DeviceResources& deviceResources,
		BackendDescriptorStore& descriptorStore,
		ID3D11Texture2D* input,
		ID3D11Texture2D** output
	);
};

}
