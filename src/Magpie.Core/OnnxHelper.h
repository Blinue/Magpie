#pragma once
#include "pch.h"
#include <onnxruntime_cxx_api.h>

namespace Magpie::Core {

struct OnnxHelper {
private:
	static void _CloseCUDAProviderOptions(OrtCUDAProviderOptionsV2* options) {
		Ort::GetApi().ReleaseCUDAProviderOptions(options);
	}

	static void _CloseTensorRTProviderOptions(OrtTensorRTProviderOptionsV2* options) {
		Ort::GetApi().ReleaseTensorRTProviderOptions(options);
	}

public:
	using unique_cuda_provider_options = wil::unique_any<OrtCUDAProviderOptionsV2*,
		decltype(_CloseCUDAProviderOptions), _CloseCUDAProviderOptions>;

	using unique_tensorrt_provider_options = wil::unique_any<OrtTensorRTProviderOptionsV2*,
		decltype(_CloseTensorRTProviderOptions), _CloseTensorRTProviderOptions>;
};

}
