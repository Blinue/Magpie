#pragma once
#include "CommandContext.h"
#include "D3D12Context.h"
#include "EffectsDrawer.h"
#include "FrameRingBuffer.h"
#include "StepTimer.h"
#include "SimpleTask.h"

namespace Magpie {

class GraphicsCaptureFrameSource;

class FrameProducer {
public:
	FrameProducer() = default;
	FrameProducer(const FrameProducer&) = delete;
	FrameProducer(FrameProducer&&) = delete;

	~FrameProducer() noexcept;

	void InitializeAsync(
		const D3D12Context& d3d12Context,
		const ColorInfo& colorInfo,
		HMONITOR hMonSrc,
		const RECT& srcRect,
		SizeU rendererSize,
		SizeU& outputSize,
		SimpleTask<bool>& task
	) noexcept;

	ComponentState GetState() const noexcept;

	uint64_t GetLatestFrameNumber() const noexcept;

	uint32_t GetFPS() const noexcept;

	bool ConsumerBeginFrame(
		ID3D12Resource*& frame,
		uint32_t& frameSrvOffset,
		uint64_t& completedFenceValue,
		uint64_t& fenceValueToSignal
	) noexcept;

	HRESULT ConsumerEndFrame(
		ID3D12CommandQueue* commandQueue,
		uint64_t fenceValueToSignal
	) const noexcept;

	void OnResizedAsync(SizeU rendererSize, SizeU& outputSize, SimpleTask<HRESULT>& task) noexcept;

	void OnColorInfoChangedAsync(const ColorInfo& colorInfo, SimpleTask<HRESULT>& task) noexcept;

	void OnCursorVisibilityChanged(bool isVisible, bool onDestory) noexcept;

private:
	void _ProducerThreadProc(
		const ColorInfo& colorInfo,
		HMONITOR hMonSrc,
		RECT srcRect,
		SizeU rendererSize,
		SizeU& outputSize,
		SimpleTask<bool>& initializeTask
	) noexcept;

	bool _Initialize(
		const ColorInfo& colorInfo,
		HMONITOR hMonSrc,
		const RECT& srcRect,
		SizeU rendererSize,
		SizeU& outputSize
	) noexcept;

	void _CreateInputDescriptors() noexcept;

	void _CreateOutputDescriptors() noexcept;

	HRESULT _Render() noexcept;

	void _MonitorThreadProc() noexcept;

	bool _CheckResult(HRESULT hr, std::string_view errorMsg) noexcept;

	std::atomic<ComponentState> _state = ComponentState::NoError;

	std::thread _producerThread;
	winrt::DispatcherQueue _dispatcher{ nullptr };

	std::thread _monitorThread;

	D3D12Context _d3d12Context;
	ComputeContext _computeContext;
	FrameRingBuffer _frameRingBuffer;
	StepTimer _stepTimer;
	std::unique_ptr<GraphicsCaptureFrameSource> _frameSource;
	EffectsDrawer _effectsDrawer;

	uint32_t _inputSrvBaseOffset = std::numeric_limits<uint32_t>::max();
	uint32_t _outputUavBaseOffset = std::numeric_limits<uint32_t>::max();
	uint32_t _outputSrvBaseOffset = std::numeric_limits<uint32_t>::max();

	bool _isScRGB = false;
};

}
