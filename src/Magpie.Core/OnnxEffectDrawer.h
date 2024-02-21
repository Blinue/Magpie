#pragma once
#include <winrt/Microsoft.AI.MachineLearning.h>

namespace Magpie::Core {

class DeviceResources;
class EffectsProfiler;

class OnnxEffectDrawer {
public:
	OnnxEffectDrawer() = default;
	OnnxEffectDrawer(const OnnxEffectDrawer&) = delete;
	OnnxEffectDrawer(OnnxEffectDrawer&&) = default;

	bool Initialize(
		const wchar_t* modelPath,
		DeviceResources& deviceResources,
		ID3D11Texture2D** inOutTexture
	) noexcept;

	void Draw(EffectsProfiler& profiler) const noexcept;

private:
	winrt::Microsoft::AI::MachineLearning::LearningModel _model{ nullptr };
	winrt::Microsoft::AI::MachineLearning::LearningModelSession _session{ nullptr };

	winrt::Windows::Media::VideoFrame _inputFrame{ nullptr };
	winrt::Windows::Media::VideoFrame _outputFrame{ nullptr };
	winrt::Windows::Media::VideoFrame _modelInputFrame{ nullptr };
	winrt::Windows::Media::VideoFrame _modelOutputFrame{ nullptr };
	
	winrt::Microsoft::AI::MachineLearning::ImageFeatureValue _inputTensor{ nullptr };
	winrt::Microsoft::AI::MachineLearning::ImageFeatureValue _outputTensor{ nullptr };
};

}
