#pragma once
#include <onnxruntime_cxx_api.h>

namespace Magpie::Core {

class DeviceResources;
class BackendDescriptorStore;

class InferenceBackendBase {
public:
	InferenceBackendBase() = default;
	virtual ~InferenceBackendBase() noexcept {}

	InferenceBackendBase(const InferenceBackendBase&) = delete;
	InferenceBackendBase(InferenceBackendBase&&) = default;

	virtual bool Initialize(
		const wchar_t* modelPath,
		DeviceResources& deviceResources,
		BackendDescriptorStore& descriptorStore,
		ID3D11Texture2D* input,
		ID3D11Texture2D** output
	) noexcept = 0;

	virtual void Evaluate() noexcept = 0;

protected:
	static void ORT_API_CALL _OrtLog(
		void* /*param*/,
		OrtLoggingLevel severity,
		const char* /*category*/,
		const char* /*logid*/,
		const char* /*code_location*/,
		const char* message
	);

	static bool _IsModelValid(const Ort::Session& session, bool& isFP16Data);
};

}
