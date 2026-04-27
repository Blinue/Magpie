#pragma once
#include "CommandContext.h"
#include "CursorDrawer.h"
#include "D3D12Context.h"
#include "DescriptorHeap.h"
#include "FrameProducer.h"
#include "OverlayDrawer.h"
#include "ScalingOptions.h"

namespace Magpie {

class SwapChainPresenter;

class Renderer {
public:
	Renderer() noexcept;
	~Renderer() noexcept;

	Renderer(const Renderer&) = delete;
	Renderer(Renderer&&) = delete;

	ScalingError Initialize(
		HWND hwndAttach,
		HMONITOR hMonitor,
		const RECT& srcRect,
		const RECT& rendererRect,
		OverlayOptions& overlayOptions,
		RECT& destRect
	) noexcept;

	ComponentState Render(
		HCURSOR hCursor,
		POINT cursorPos,
		bool waitForGpu = false,
		bool* waitingForFirstFrame = nullptr
	) noexcept;

	const RectU& GetOutputRect() const noexcept {
		return _outputRect;
	}

	void OnMonitorChanged(HMONITOR hMonitor) noexcept;

	void OnResizingChanged(bool value) noexcept;

	void OnResized(const RECT& rendererRect, RECT& destRect) noexcept;

	void OnMovingChanged(bool value) noexcept;

	void OnMoved(const RECT& rendererRect, RECT& destRect) noexcept;

	void OnCursorVirtualizationChanged(bool value) noexcept;

	void OnCursorCapturedOnForegroundChanged(bool value) noexcept;

	void OnSrcMovingChanged(bool value) noexcept;

	void OnMsgDisplayChanged() noexcept;

	void OnCursorVisibilityChanged(bool isVisible, bool onDestory) noexcept;

private:
	void _TryInitDisplayInfo() noexcept;

	bool _UpdateColorInfo() noexcept;

	HRESULT _UpdateColorSpace() noexcept;

	HRESULT _RenderImpl(POINT cursorPos, bool waitForGpu = false) noexcept;

	void _UpdateOutputRect(SizeU outputSize) noexcept;

	bool _CheckResult(bool success, std::string_view errorMsg) noexcept;

	bool _CheckResult(HRESULT hr, std::string_view errorMsg) noexcept;

	HRESULT _CreateCopyFramePSO(bool isSrgb, winrt::com_ptr<ID3D12PipelineState>& result) noexcept;

	// 不使用 Initializing 状态
	ComponentState _state = ComponentState::NoError;

	winrt::DisplayInformation _displayInfo{ nullptr };
	winrt::DisplayInformation::AdvancedColorInfoChanged_revoker _acInfoChangedRevoker;

	RectU _outputRect{};

	// 由多个 D3D12Context 共享
	DescriptorHeap _csuDescriptorHeap;
	DescriptorHeap _rtvDescriptorHeap;
	D3D12Context _d3d12Context;
	GraphicsContext _graphicsContext;
	FrameProducer _frameProducer;
	OverlayDrawer _overlayDrawer;
	CursorDrawer _cursorDrawer;
	std::unique_ptr<SwapChainPresenter> _presenter;
	
	HMONITOR _hCurMonitor = NULL;
	ColorInfo _colorInfo;

	uint64_t _lastProducerFrameNumber = 0;

	winrt::com_ptr<ID3D12RootSignature> _copyRootSignature;
	winrt::com_ptr<ID3D12PipelineState> _copyFramePSO;
	winrt::com_ptr<ID3D12PipelineState> _copyFrameSrgbPSO;
};

}
