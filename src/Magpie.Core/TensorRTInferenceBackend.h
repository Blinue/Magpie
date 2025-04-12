#pragma once
#include "InferenceBackendBase.h"

#ifdef _M_X64

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

	ID3D11SamplerState* _sampler = nullptr;
	ID3D11ShaderResourceView* _inputTexSrv = nullptr;
	winrt::com_ptr<ID3D11UnorderedAccessView> _inputBufferUav;
	winrt::com_ptr<ID3D11ShaderResourceView> _outputBufferSrv;
	winrt::com_ptr<ID3D11UnorderedAccessView> _outputTexUav;

	winrt::com_ptr<IDXGIKeyedMutex> _inputBufferKmt;
	winrt::com_ptr<IDXGIKeyedMutex> _outputBufferKmt;

	UINT64 _inputBufferMutexKey = 0;
	UINT64 _outputBufferMutexKey = 0;

	winrt::com_ptr<ID3D11ComputeShader> _texToTensorShader;
	winrt::com_ptr<ID3D11ComputeShader> _tensorToTexShader;

	std::pair<uint32_t, uint32_t> _texToTensorDispatchCount{};
	std::pair<uint32_t, uint32_t> _tensorToTexDispatchCount{};

	Ort::MemoryInfo _cudaMemInfo{ nullptr };

	// cudaExternalMemory_t
	void* _inputBufferCudaMem = nullptr;
	void* _outputBufferCudaMem = nullptr;
	void* _inputBufferCudaPtr = nullptr;
	void* _outputBufferCudaPtr = nullptr;
	// cudaExternalSemaphore_t
	void* _inputBufferCudaSem = nullptr;
	void* _outputBufferCudaSem = nullptr;

	Ort::IoBinding _ioBinding{ nullptr };
};

}

#endif
