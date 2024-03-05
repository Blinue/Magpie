#pragma once
#include <onnxruntime_cxx_api.h>

namespace Magpie::Core {

class DeviceResources;
class BackendDescriptorStore;

class DirectMLInferenceBackend {
public:
	DirectMLInferenceBackend() = default;
	DirectMLInferenceBackend(const DirectMLInferenceBackend&) = delete;
	DirectMLInferenceBackend(DirectMLInferenceBackend&&) = default;

	bool Initialize(
		const wchar_t* modelPath,
		DeviceResources& deviceResources,
		BackendDescriptorStore& descriptorStore,
		ID3D11Texture2D* input,
		ID3D11Texture2D** output
	) noexcept;

	void Evaluate() noexcept;

private:
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

	Ort::Env _env{ nullptr };
	Ort::Session _session{ nullptr };

	SIZE _inputSize{};
};

}
