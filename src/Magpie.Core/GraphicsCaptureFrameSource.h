#pragma once
#include "SmallVector.h"
#include <d3d11_4.h>
#include <ShlObj.h>
#include <winrt/Windows.Graphics.Capture.h>

namespace Magpie {

class D3D12Context;
class DuplicateFrameChecker;

enum class FrameSourceState {
	WaitingForFirstFrame,
	Waiting,
	NewFrameAvailable
};

// 使用 Windows.Graphics.Capture 接口捕获窗口，见
// https://docs.microsoft.com/en-us/windows/uwp/audio-video-camera/screen-capture
class GraphicsCaptureFrameSource {
public:
	GraphicsCaptureFrameSource() = default;
	GraphicsCaptureFrameSource(const GraphicsCaptureFrameSource&) = delete;
	GraphicsCaptureFrameSource(GraphicsCaptureFrameSource&&) = delete;

	~GraphicsCaptureFrameSource() noexcept;

	bool Initialize(
		D3D12Context& d3d12Context,
		const RECT& srcRect,
		HMONITOR hMonSrc,
		const ColorInfo& colorInfo
	) noexcept;

	bool Start() noexcept;

	ID3D12Resource* GetOutput(uint32_t index) noexcept {
		return _slots[index].output.get();
	}

	bool ShouldWaitMessageForNewFrame() const noexcept {
		return true;
	}

	HRESULT CheckForNewFrame(bool& isNewFrameAvailable) noexcept;

	HRESULT Update(uint32_t& outputIdx) noexcept;

	HRESULT OnColorInfoChanged(const ColorInfo& colorInfo) noexcept;

	HRESULT OnCursorVisibilityChanged(bool isVisible, bool onDestory) noexcept;

private:
	bool _CreateCaptureDevice(HMONITOR hMonSrc) noexcept;

	bool _CreateBridgeDeviceResources(IDXGIAdapter1* dxgiAdapter) noexcept;

	HRESULT _CreateDisplayDependentResources() noexcept;

	bool _InitializeCaptureItem() noexcept;

	void _Direct3D11CaptureFramePool_FrameArrived(
		const winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool& pool,
		const winrt::IInspectable&
	);

	void _DisableRoundCornerInWin11() noexcept;

	HRESULT _StartCapture() noexcept;

	void _StopCapture() noexcept;

	D3D12Context* _d3d12Context = nullptr;

	std::atomic<DWORD> _producerThreadId;

	winrt::com_ptr<ID3D11Device5> _d3d11Device;
	winrt::com_ptr<ID3D11DeviceContext4> _d3d11DC;

	winrt::com_ptr<ID3D12CommandQueue> _copyCommandQueue;
	winrt::com_ptr<ID3D12GraphicsCommandList> _copyCommandList;

	// 用于跨适配器捕获
	winrt::com_ptr<ID3D12Device5> _bridgeDevice;
	winrt::com_ptr<ID3D12CommandQueue> _bridgeCopyCommandQueue;
	winrt::com_ptr<ID3D12GraphicsCommandList> _bridgeCopyCommandList;
	winrt::com_ptr<ID3D12Heap> _bridgeHeap;
	winrt::com_ptr<ID3D12Heap> _sharedHeap;
	winrt::com_ptr<ID3D12Fence1> _bridgeFence;
	winrt::com_ptr<ID3D12Fence1> _sharedFence;
	uint64_t _curCrossAdapterFenceValue = 0;

	wil::srwlock _latestFrameLock;
	// 不要在持有 _latestFrameLock 时释放 _latestFrame 或调用其中的方法，和 WGC 内部的
	// 同步机制冲突。如果此时_Direct3D11CaptureFramePool_FrameArrived 正在执行会死锁。
	winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame _latestFrame{ nullptr };
	SmallVector<RectU> _latestFrameDirtyRects;

	std::vector<std::pair<ID3D11Texture2D*, winrt::com_ptr<ID3D12Resource>>> _captureFrameResourceTable;
	std::unique_ptr<DuplicateFrameChecker> _duplicateFrameChecker;
	winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame _newFrame{ nullptr };
	SmallVector<RectU> _newFrameDirtyRects;
	uint32_t _newCaptureFrameResourceIdx = 0;

	struct _FrameCrossAdapterResourceSlot {
		winrt::com_ptr<ID3D12CommandAllocator> commandAllocator;
		winrt::com_ptr<ID3D12Resource> bridgeResource;
		winrt::com_ptr<ID3D12Resource> sharedResource;
	};

	std::vector<_FrameCrossAdapterResourceSlot> _crossAdapterSlots;
	
	winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice _wrappedDevice{ nullptr };
	winrt::Windows::Graphics::Capture::GraphicsCaptureItem _captureItem{ nullptr };
	winrt::Windows::Graphics::Capture::GraphicsCaptureSession _captureSession{ nullptr };
	winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool _captureFramePool{ nullptr };
	winrt::com_ptr<ITaskbarList> _taskbarList;
	
	struct _FrameResourceSlot {
		winrt::com_ptr<ID3D12CommandAllocator> commandAllocator;
		// 保留引用防止 WGC 再次写入
		winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame captureFrame{ nullptr };
		uint32_t captureFrameResourceIdx = 0;
		SmallVector<RectU> dirtyRects;
		winrt::com_ptr<ID3D12Resource> output;
	};

	std::vector<_FrameResourceSlot> _slots;
	uint32_t _curFrameIdx = 0;
	
	D3D12_BOX _frameBox{};

	bool _isScRGB = false;
	bool _isSrcStyleChanged = false;
	bool _isRoundCornerDisabled = false;
	bool _isDirtyRegionSupported = false;
};

}
