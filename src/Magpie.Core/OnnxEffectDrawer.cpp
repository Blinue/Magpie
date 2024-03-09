#include "pch.h"
#include "OnnxEffectDrawer.h"
#include "Logger.h"
#include "TensorRTInferenceBackend.h"
#include "DirectMLInferenceBackend.h"

namespace Magpie::Core {

OnnxEffectDrawer::OnnxEffectDrawer() {}

OnnxEffectDrawer::~OnnxEffectDrawer() {}

bool OnnxEffectDrawer::Initialize(
	const wchar_t* modelPath,
	DeviceResources& deviceResources,
	BackendDescriptorStore& descriptorStore,
	ID3D11Texture2D** inOutTexture
) noexcept {
	_inferenceBackend = std::make_unique<DirectMLInferenceBackend>();

	if (!_inferenceBackend->Initialize(modelPath, deviceResources, descriptorStore, *inOutTexture, inOutTexture)) {
		return false;
	}

	return true;
}

void OnnxEffectDrawer::Draw(EffectsProfiler& /*profiler*/) const noexcept {
	_inferenceBackend->Evaluate();
}

}
