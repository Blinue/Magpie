#pragma once

namespace Magpie {

class D3D12Context;

class SwapChainPresenter {
public:
	SwapChainPresenter() = default;
	SwapChainPresenter(const SwapChainPresenter&) = delete;
	SwapChainPresenter(SwapChainPresenter&&) = delete;

	~SwapChainPresenter() noexcept;

	bool Initialize(
		D3D12Context& graphicContext,
		HWND hwndAttach,
		SizeU size,
		const ColorInfo& colorInfo
	) noexcept;

	// SDR 下 rawRtvOffse 不做伽马校正，用于渲染光标
	void BeginFrame(
		ID3D12Resource** backBuffer,
		uint32_t& rtvOffset,
		uint32_t& rawRtvOffse
	) noexcept;

	HRESULT EndFrame(bool waitForGpu = false) noexcept;

	SizeU GetSize() const noexcept { return _size; }

	HRESULT OnResizingChanged(bool value) noexcept;

	HRESULT OnResized(SizeU size) noexcept;

	HRESULT OnColorInfoChanged(const ColorInfo& colorInfo) noexcept;

private:
	HRESULT _RecreateBuffers() noexcept;

	HRESULT _CreateDisplayDependentResources() noexcept;

	D3D12Context* _d3d12Context = nullptr;

	winrt::com_ptr<IDXGISwapChain4> _dxgiSwapChain;
	wil::unique_event_nothrow _frameLatencyWaitableObject;
	std::vector<winrt::com_ptr<ID3D12Resource>> _frameBuffers;

	SizeU _size{};
	uint32_t _bufferCount = 0;
	uint32_t _rtvBaseOffset = std::numeric_limits<uint32_t>::max();
	uint32_t _rawRtvBaseOffset = std::numeric_limits<uint32_t>::max();
	
	bool _isScRGB = false;
	bool _isTearingSupported = false;
	bool _isRecreated = true;
	bool _isResizing = false;
};

}
