#pragma once
#include <winrt/Microsoft.AI.MachineLearning.h>

namespace Magpie::Core {

class DeviceResources;
class EffectsProfiler;
class TensorRTInferenceEngine;

class OnnxEffectDrawer {
public:
	OnnxEffectDrawer();
	OnnxEffectDrawer(const OnnxEffectDrawer&) = delete;
	OnnxEffectDrawer(OnnxEffectDrawer&&) = default;

	~OnnxEffectDrawer();

	bool Initialize(
		const wchar_t* modelPath,
		DeviceResources& deviceResources,
		ID3D11Texture2D** inOutTexture
	) noexcept;

	void Draw(EffectsProfiler& profiler) const noexcept;

private:
	winrt::Microsoft::AI::MachineLearning::LearningModelSession _session{ nullptr };

	std::wstring _inputName;
	std::wstring _outputName;
	
	winrt::Microsoft::AI::MachineLearning::ImageFeatureValue _inputTensor{ nullptr };
	winrt::Microsoft::AI::MachineLearning::ImageFeatureValue _outputTensor{ nullptr };

	std::unique_ptr<TensorRTInferenceEngine> _inferenceEngine;
};

}
