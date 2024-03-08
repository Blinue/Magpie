#pragma once
#include <onnxruntime_cxx_api.h>
#include <d3d12.h>

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
	ID3D11DeviceContext4* _d3d11DC = nullptr;

	winrt::com_ptr<ID3D11Texture2D> _outputTex;

	winrt::com_ptr<ID3D11Fence> _d3d11Fence;
	winrt::com_ptr<ID3D12Fence> _d3d12Fence;
	UINT64 _fenceValue = 0;

	winrt::com_ptr<ID3D12Resource> _d3d12InputTex;
	winrt::com_ptr<ID3D12Resource> _d3d12OutputTex;
	winrt::com_ptr<ID3D12Resource> _inputBuffer;
	winrt::com_ptr<ID3D12Resource> _outputBuffer;

	winrt::com_ptr<ID3D12DescriptorHeap> _cbvHeap;
	winrt::com_ptr<ID3D12RootSignature> _rootSignature;
	winrt::com_ptr<ID3D12PipelineState> _tex2TensorPipelineState;
	winrt::com_ptr<ID3D12PipelineState> _tensor2TexPipelineState;

	winrt::com_ptr<ID3D12CommandQueue> _commandQueue;
	winrt::com_ptr<ID3D12GraphicsCommandList> _tex2TensorCommandList;
	winrt::com_ptr<ID3D12GraphicsCommandList> _tensor2TexCommandList;

	Ort::Env _env{ nullptr };
	Ort::Session _session{ nullptr };

	winrt::com_ptr<IUnknown> _allocatedInput;
	winrt::com_ptr<IUnknown> _allocatedOutput;

	Ort::IoBinding _ioBinding{ nullptr };
};

}
