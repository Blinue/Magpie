#pragma once
#include "pch.h"
#include <onnxruntime_cxx_api.h>

namespace Magpie {

struct OnnxHelper {
private:
	static void _CloseCUDAProviderOptions(OrtCUDAProviderOptionsV2* options) {
		Ort::GetApi().ReleaseCUDAProviderOptions(options);
	}

	static void _CloseTensorRTProviderOptions(OrtTensorRTProviderOptionsV2* options) {
		Ort::GetApi().ReleaseTensorRTProviderOptions(options);
	}

public:
	// 上游删除了 DirectXHelper::GetTextureSize / upstream removed
	// DirectXHelper::GetTextureSize; it now reads the desc inline.
	static SIZE GetTextureSize(ID3D11Texture2D* texture) noexcept {
		D3D11_TEXTURE2D_DESC desc;
		texture->GetDesc(&desc);
		return SIZE{ (LONG)desc.Width, (LONG)desc.Height };
	}

	using unique_cuda_provider_options = wil::unique_any<OrtCUDAProviderOptionsV2*,
		decltype(_CloseCUDAProviderOptions), _CloseCUDAProviderOptions>;

	using unique_tensorrt_provider_options = wil::unique_any<OrtTensorRTProviderOptionsV2*,
		decltype(_CloseTensorRTProviderOptions), _CloseTensorRTProviderOptions>;
};

}
