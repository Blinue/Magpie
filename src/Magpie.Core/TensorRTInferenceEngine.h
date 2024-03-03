#pragma once
/*#pragma warning(push)
// C4100: “pluginFactory”: 未引用的形参
// C4996: 'nvinfer1::IPluginV2' : 被声明为已否决
#pragma warning(disable: 4100 4996)
#include <NvInfer.h>
#include <NvInferPlugin.h>
#pragma warning(pop)*/
#include <onnxruntime/core/session/onnxruntime_cxx_api.h>

struct cudaGraphicsResource;

namespace Magpie::Core {

class DeviceResources;
class BackendDescriptorStore;

/*struct TensorRTLogger : public nvinfer1::ILogger {
	void log(Severity severity, nvinfer1::AsciiChar const* msg) noexcept override;
};*/

class TensorRTInferenceEngine {
public:
	TensorRTInferenceEngine() = default;
	TensorRTInferenceEngine(const TensorRTInferenceEngine&) = delete;
	TensorRTInferenceEngine(TensorRTInferenceEngine&&) = default;

	~TensorRTInferenceEngine();

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

	Ort::Env _env{ OrtLoggingLevel::ORT_LOGGING_LEVEL_INFO, "test"};
	Ort::Session _session{ nullptr };
	Ort::MemoryInfo _cudaMemInfo{ nullptr };

	SIZE _inputSize{};

	const char* _inputName = nullptr;
	const char* _outputName = nullptr;

	cudaGraphicsResource* _inputBufferCuda = nullptr;
	cudaGraphicsResource* _outputBufferCuda = nullptr;
};

}
