#pragma once
#include "CudaInferenceBackend.h"

namespace Magpie::Core {

class TensorRTInferenceBackend : public CudaInferenceBackend {
public:
	TensorRTInferenceBackend() = default;
	TensorRTInferenceBackend(const TensorRTInferenceBackend&) = delete;
	TensorRTInferenceBackend(TensorRTInferenceBackend&&) = default;

protected:
	bool _CheckComputeCapability(int deviceId) noexcept override;

	bool _CreateSession(
		DeviceResources& deviceResources,
		int deviceId,
		Ort::SessionOptions& sessionOptions,
		const wchar_t* modelPath
	) override;
};

}
