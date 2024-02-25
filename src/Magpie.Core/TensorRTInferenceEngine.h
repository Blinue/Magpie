#pragma once

namespace Magpie::Core {

class DeviceResources;

class TensorRTInferenceEngine {
public:
	TensorRTInferenceEngine() = default;
	TensorRTInferenceEngine(const TensorRTInferenceEngine&) = delete;
	TensorRTInferenceEngine(TensorRTInferenceEngine&&) = default;

	bool Initialize(
		const wchar_t* modelPath,
		DeviceResources& deviceResources,
		ID3D11Texture2D* input,
		ID3D11Texture2D** output
	);
};

}
