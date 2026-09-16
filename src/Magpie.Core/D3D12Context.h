#pragma once
#include "ScalingOptions.h"

namespace Magpie {

class DescriptorHeap;

class D3D12Context {
public:
	D3D12Context() = default;
	D3D12Context(const D3D12Context&) = delete;
	D3D12Context(D3D12Context&&) = delete;

	bool Initialize(
		const GraphicsCardId& graphicsCardId,
		uint32_t maxInFlightFrameCount,
		D3D12_COMMAND_QUEUE_PRIORITY priority,
		D3D12_COMMAND_LIST_TYPE commandListType,
		DescriptorHeap& csuDescriptorHeap,
		DescriptorHeap& rtvDescriptorHeap,
		bool disableFrameFenceTracking = false
	) noexcept;

	void CopyDevice(const D3D12Context& other);

	bool InitializeAfterCopyDevice(
		uint32_t maxInFlightFrameCount,
		D3D12_COMMAND_QUEUE_PRIORITY priority,
		D3D12_COMMAND_LIST_TYPE commandListType,
		bool disableFrameTracking = false
	) noexcept;

	DescriptorHeap& GetDescriptorHeap(bool rtv = false) const noexcept {
		return rtv ? *_rtvDescriptorHeap : *_csuDescriptorHeap;
	}

	IDXGIFactory7* GetDXGIFactory() const noexcept {
		return _dxgiFactory.get();
	}

	IDXGIFactory7* GetDXGIFactoryForEnumingAdapters() noexcept;

	IDXGIAdapter4* GetDXGIAdapter() const noexcept {
		return _dxgiAdapter.get();
	}

	ID3D12Device5* GetDevice() const noexcept {
		return _device.get();
	}

	ID3D12CommandQueue* GetCommandQueue() const noexcept {
		return _commandQueue.get();
	}

	ID3D12GraphicsCommandList* GetCommandList() const noexcept {
		return _commandList.get();
	}

	D3D_SHADER_MODEL GetShaderModel() const noexcept {
		return _shaderModel;
	}

	D3D_ROOT_SIGNATURE_VERSION GetRootSignatureVersion() const noexcept {
		return _rootSignatureVersion;
	}

	bool IsUMA() const noexcept {
		return _isUMA;
	}

	bool IsHeapFlagCreateNotZeroedSupported() const noexcept {
		return _isHeapFlagCreateNotZeroedSupported;
	}

	bool IsGPUUploadHeapSupported() const noexcept {
		return _isGPUUploadHeapSupported;
	}

	bool IsMinFloat16Supported() const noexcept {
		return _isMinFloat16Supported;
	}

	bool IsNative16BitSupported() const noexcept {
		return _isNative16BitSupported;
	}

	uint32_t GetMaxInFlightFrameCount() const noexcept {
		return (uint32_t)_commandAllocators.size();
	}

	HRESULT Signal(uint64_t& fenceValue) noexcept;

	HRESULT WaitForFenceValue(uint64_t fenceValue) noexcept;

	HRESULT WaitForGpu() noexcept;

	HRESULT WaitForCommandQueue(ID3D12CommandQueue* commandQueue) noexcept;

	HRESULT BeginFrame(
		uint32_t& curFrameIndex,
		ID3D12PipelineState* initialState = nullptr
	) noexcept;

	HRESULT EndFrame() noexcept;

private:
	HRESULT _CreateDXGIFactory() noexcept;

	bool _InitializeDeviceResources(
		uint32_t maxInFlightFrameCount,
		D3D12_COMMAND_QUEUE_PRIORITY priority,
		D3D12_COMMAND_LIST_TYPE commandListType,
		bool disableFrameFenceTracking
	) noexcept;

	bool _CreateAdapterAndDevice(const GraphicsCardId& graphicsCardId) noexcept;

	bool _TryCreateD3DDevice(
		const winrt::com_ptr<IDXGIAdapter1>& adapter,
		const DXGI_ADAPTER_DESC1& adapterDesc
	) noexcept;

	bool _CreateAdapterFromDevice() noexcept;

	void _QueryHighestShaderModel() noexcept;

	void _LogDeviceInfo() noexcept;

	DescriptorHeap* _csuDescriptorHeap = nullptr;
	DescriptorHeap* _rtvDescriptorHeap = nullptr;

	winrt::com_ptr<IDXGIFactory7> _dxgiFactory;
	winrt::com_ptr<IDXGIAdapter4> _dxgiAdapter;
	winrt::com_ptr<ID3D12Device5> _device;
	winrt::com_ptr<ID3D12CommandQueue> _commandQueue;

	std::vector<winrt::com_ptr<ID3D12CommandAllocator>> _commandAllocators;
	winrt::com_ptr<ID3D12GraphicsCommandList> _commandList;

	winrt::com_ptr<ID3D12Fence1> _fence;
	uint64_t _curFenceValue = 0;

	std::vector<uint64_t> _frameFenceValues;
	uint32_t _curFrameIndex = 0;

	D3D_SHADER_MODEL _shaderModel = D3D_SHADER_MODEL_5_1;
	D3D_ROOT_SIGNATURE_VERSION _rootSignatureVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	
	bool _isUMA = false;
	bool _isHeapFlagCreateNotZeroedSupported = false;
	bool _isGPUUploadHeapSupported = false;
	bool _isMinFloat16Supported = false;
	bool _isNative16BitSupported = false;
};

}
