#pragma once
#include "InferenceBackendBase.h"

struct cudaGraphicsResource;

namespace Magpie::Core {

class TensorRTInferenceBackend : public InferenceBackendBase {
public:
	TensorRTInferenceBackend() = default;
	TensorRTInferenceBackend(const TensorRTInferenceBackend&) = delete;
	TensorRTInferenceBackend(TensorRTInferenceBackend&&) = default;

	virtual ~TensorRTInferenceBackend();

	bool Initialize(
		const wchar_t* modelPath,
		uint32_t scale,
		DeviceResources& deviceResources,
		BackendDescriptorStore& descriptorStore,
		ID3D11Texture2D* input,
		ID3D11Texture2D** output
	) noexcept override;

	void Evaluate() noexcept override;

private:
	bool _CreateSession(
		DeviceResources& deviceResources,
		int deviceId,
		Ort::SessionOptions& sessionOptions,
		const wchar_t* modelPath
	);

	Ort::Env _env{ nullptr };
	Ort::Session _session{ nullptr };

	ID3D11DeviceContext4* _d3dDC = nullptr;

	ID3D11SamplerState* _pointSampler = nullptr;
	ID3D11ShaderResourceView* _inputTexSrv = nullptr;
	winrt::com_ptr<ID3D11UnorderedAccessView> _inputBufferUav;
	winrt::com_ptr<ID3D11ShaderResourceView> _outputBufferSrv;
	winrt::com_ptr<ID3D11UnorderedAccessView> _outputTexUav;

	winrt::com_ptr<ID3D11ComputeShader> _texToTensorShader;
	winrt::com_ptr<ID3D11ComputeShader> _tensorToTexShader;

	std::pair<uint32_t, uint32_t> _texToTensorDispatchCount{};
	std::pair<uint32_t, uint32_t> _tensorToTexDispatchCount{};

	Ort::MemoryInfo _cudaMemInfo{ nullptr };

	SIZE _inputSize{};
	SIZE _outputSize{};

	const char* _inputName = nullptr;
	const char* _outputName = nullptr;

	cudaGraphicsResource* _inputBufferCuda = nullptr;
	cudaGraphicsResource* _outputBufferCuda = nullptr;

	bool _isFP16Data = false;
};

}
