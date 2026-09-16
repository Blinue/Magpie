#include "pch.h"
#include "FrameProducer.h"
#include "CommonSharedConstants.h"
#include "DuplicateFrameChecker.h"
#include "DescriptorHeap.h"
#include "GraphicsCaptureFrameSource.h"
#include "Logger.h"
#include "ScalingWindow.h"
#include <dispatcherqueue.h>

namespace Magpie {

FrameProducer::~FrameProducer() noexcept {
	if (_producerThread.joinable()) {
		const HANDLE hThread = _producerThread.native_handle();

		if (!wil::handle_wait(hThread, 0)) {
			const DWORD threadId = GetThreadId(_producerThread.native_handle());

			while (true) {
				// 持续尝试直到 _producerThread 创建了消息队列
				PostThreadMessage(threadId, WM_QUIT, 0, 0);

				if (wil::handle_wait(hThread, 1)) {
					break;
				}
			}
		}

		_producerThread.join();
	}

#ifdef _DEBUG
	if (_inputSrvBaseOffset != std::numeric_limits<uint32_t>::max()) {
		auto& descriptorHeap = _d3d12Context.GetDescriptorHeap();
		uint32_t maxInFlightFrameCount = ScalingWindow::Get().Options().maxProducerInFlightFrames;
		descriptorHeap.Free(_inputSrvBaseOffset, 3 * maxInFlightFrameCount + 2);
	}
#endif
}

void FrameProducer::InitializeAsync(
	const D3D12Context& d3d12Context,
	const ColorInfo& colorInfo,
	HMONITOR hMonSrc,
	const RECT& srcRect,
	SizeU rendererSize,
	SizeU& outputSize,
	SimpleTask<bool>& task
) noexcept {
	_d3d12Context.CopyDevice(d3d12Context);

	_producerThread = std::thread(
		&FrameProducer::_ProducerThreadProc,
		this,
		colorInfo,
		hMonSrc,
		srcRect,
		rendererSize,
		std::ref(outputSize),
		std::ref(task)
	);
}

ComponentState FrameProducer::GetState() const noexcept {
	return _state.load(std::memory_order_relaxed);
}

uint64_t FrameProducer::GetLatestFrameNumber() const noexcept {
	return _frameRingBuffer.GetLatestFrameNumber();
}

uint32_t FrameProducer::GetFPS() const noexcept {
	return _stepTimer.GetFPS();
}

bool FrameProducer::ConsumerBeginFrame(
	ID3D12Resource*& frame,
	uint32_t& frameSrvOffset,
	uint64_t& completedFenceValue,
	uint64_t& fenceValueToSignal
) noexcept {
	uint32_t bufferIdx;
	if (!_frameRingBuffer.ConsumerBeginFrame(
		bufferIdx, frame, completedFenceValue, fenceValueToSignal)) {
		return false;
	}

	frameSrvOffset = _outputSrvBaseOffset + bufferIdx;
	return true;
}

HRESULT FrameProducer::ConsumerEndFrame(
	ID3D12CommandQueue* commandQueue,
	uint64_t fenceValueToSignal
) const noexcept {
	return _frameRingBuffer.ConsumerEndFrame(commandQueue, fenceValueToSignal);
}

void FrameProducer::OnResizedAsync(
	SizeU rendererSize,
	SizeU& outputSize,
	SimpleTask<HRESULT>& task
) noexcept {
	_dispatcher.TryEnqueue([&, rendererSize] {
		HRESULT hr = S_OK;
		auto se = wil::scope_exit([&] {
			// 同步 outputSize
			task.SetResult(hr, std::memory_order_release);
		});

		ComponentState state = _state.load(std::memory_order_relaxed);
		if (state != ComponentState::NoError) {
			hr = state == ComponentState::DeviceLost ? DXGI_ERROR_DEVICE_REMOVED : E_FAIL;
			return;
		}

		hr = _d3d12Context.WaitForGpu();
		if (!_CheckResult(hr, "D3D12Context::WaitForGpu 失败")) {
			return;
		}

		_effectsDrawer.OnResized(rendererSize, outputSize);

		hr = _frameRingBuffer.OnResized(outputSize);
		if (!_CheckResult(hr, "FrameRingBuffer::OnResized 失败")) {
			return;
		}

		_CreateOutputDescriptors();

		hr = _Render();
		if (!_CheckResult(hr, "_Render 失败")) {
			return;
		}

		// 等待渲染完成
		hr = _d3d12Context.WaitForGpu();
		if (!_CheckResult(hr, "D3D12Context::WaitForGpu 失败")) {
			return;
		}
	});
}

void FrameProducer::OnColorInfoChangedAsync(
	const ColorInfo& colorInfo,
	SimpleTask<HRESULT>& task
) noexcept {
	_dispatcher.TryEnqueue([&] {
		HRESULT hr = S_OK;
		auto se = wil::scope_exit([&] {
			task.SetResult(hr);
		});
		
		ComponentState state = _state.load(std::memory_order_relaxed);
		if (state != ComponentState::NoError) {
			hr = state == ComponentState::DeviceLost ? DXGI_ERROR_DEVICE_REMOVED : E_FAIL;
			return;
		}

		_isScRGB = colorInfo.kind != winrt::AdvancedColorKind::StandardDynamicRange;

		hr = _d3d12Context.WaitForGpu();
		if (!_CheckResult(hr, "D3D12Context::WaitForGpu 失败")) {
			return;
		}

		hr = _frameSource->OnColorInfoChanged(colorInfo);
		if (!_CheckResult(hr, "GraphicsCaptureFrameSource::OnColorInfoChanged 失败")) {
			return;
		}

		_effectsDrawer.OnColorInfoChanged(colorInfo);

		hr = _frameRingBuffer.OnColorInfoChanged(colorInfo);
		if (!_CheckResult(hr, "FrameRingBuffer::OnColorInfoChanged 失败")) {
			return;
		}

		_CreateInputDescriptors();
		_CreateOutputDescriptors();
		
		// 等待新帧
		while (true) {
			bool isNewFrameAvailable;
			hr = _frameSource->CheckForNewFrame(isNewFrameAvailable);
			if (!_CheckResult(hr, "GraphicsCaptureFrameSource::CheckForNewFrame 失败")) {
				return;
			}

			if (isNewFrameAvailable) {
				break;
			} else {
				WaitMessage();
			}
		}

		hr = _Render();
		if (!_CheckResult(hr, "_Render 失败")) {
			return;
		}

		// 等待渲染完成
		hr = _d3d12Context.WaitForGpu();
		if (!_CheckResult(hr, "D3D12Context::WaitForGpu 失败")) {
			return;
		}
	});
}

void FrameProducer::OnCursorVisibilityChanged(bool isVisible, bool onDestory) noexcept {
	_dispatcher.TryEnqueue([this, isVisible, onDestory] {
		if (_state.load(std::memory_order_relaxed) != ComponentState::NoError) {
			return;
		}

		_CheckResult(_frameSource->OnCursorVisibilityChanged(isVisible, onDestory),
			"GraphicsCaptureFrameSource::OnCursorVisibilityChanged 失败");
	});
}

void FrameProducer::_ProducerThreadProc(
	const ColorInfo& colorInfo,
	HMONITOR hMonSrc,
	RECT srcRect,
	SizeU rendererSize,
	SizeU& outputSize,
	SimpleTask<bool>& initializeTask
) noexcept {
#ifdef _DEBUG
	SetThreadDescription(GetCurrentThread(), L"Magpie-缩放生产者线程");
#endif

	if (_Initialize(colorInfo, hMonSrc, srcRect, rendererSize, outputSize)) {
		// 同步 outputSize
		initializeTask.SetResult(true, std::memory_order_release);
	} else {
		Logger::Get().Error("_Initialize 失败");
		initializeTask.SetResult(false);
		return;
	}

	StepTimerStatus stepTimerStatus = StepTimerStatus::WaitingForNewFrame;
	const bool waitMsgForNewFrame = _frameSource->ShouldWaitMessageForNewFrame();
	bool isWaitingForFirstFrame = true;

	MSG msg;
	while (true) {
		bool fpsUpdated = false;
		// WaitingForFPSLimiter 状态下新帧消息可能已被处理，不要等待消息，直到状态变化
		stepTimerStatus = _stepTimer.WaitForNextFrame(
			waitMsgForNewFrame && stepTimerStatus != StepTimerStatus::WaitingForFPSLimiter,
			fpsUpdated
		);

		if (fpsUpdated) {
			// FPS 变化时要求前端重新渲染以更新叠加层
			PostMessage(ScalingWindow::Get().Handle(),
				CommonSharedConstants::WM_FRONTEND_RENDER, 0, 0);
		}

		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				break;
			}

			DispatchMessage(&msg);
		}

		// 异步检查回调是否出错
		if (msg.message == WM_QUIT ||
			_state.load(std::memory_order_relaxed) != ComponentState::NoError) {
			break;
		}

		if (stepTimerStatus == StepTimerStatus::WaitingForFPSLimiter) {
			continue;
		}

		bool isNewFrameAvailable;
		if (!_CheckResult(_frameSource->CheckForNewFrame(isNewFrameAvailable),
			"GraphicsCaptureFrameSource::CheckForNewFrame 失败")) {
			break;
		}

		// 强制等待第一帧
		if (!isNewFrameAvailable && (isWaitingForFirstFrame || stepTimerStatus != StepTimerStatus::ForceNewFrame)) {
			continue;
		}
		isWaitingForFirstFrame = false;

		if (!_CheckResult(_Render(), "_Render 失败")) {
			break;
		}
	}

	_d3d12Context.WaitForGpu();
	// 必须在创建线程释放
	_frameSource.reset();

	if (_monitorThread.joinable()) {
		const HANDLE hThread = _monitorThread.native_handle();

		if (!wil::handle_wait(hThread, 0)) {
			const DWORD threadId = GetThreadId(_monitorThread.native_handle());

			while (true) {
				// 持续尝试直到创建了消息队列
				PostThreadMessage(threadId, WM_QUIT, 0, 0);

				if (wil::handle_wait(hThread, 1)) {
					break;
				}
			}
		}

		_monitorThread.join();
	}
}

bool FrameProducer::_Initialize(
	const ColorInfo& colorInfo,
	HMONITOR hMonSrc,
	const RECT& srcRect,
	SizeU rendererSize,
	SizeU& outputSize
) noexcept {
	winrt::init_apartment(winrt::apartment_type::single_threaded);

	// 创建 DispatcherQueue
	{
		winrt::Windows::System::DispatcherQueueController dqc{ nullptr };
		HRESULT hr = CreateDispatcherQueueController(
			DispatcherQueueOptions{
				.dwSize = sizeof(DispatcherQueueOptions),
				.threadType = DQTYPE_THREAD_CURRENT
			},
			(PDISPATCHERQUEUECONTROLLER*)winrt::put_abi(dqc)
		);
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateDispatcherQueueController 失败", hr);
			return false;
		}

		_dispatcher = dqc.DispatcherQueue();
	}

	const ScalingOptions& options = ScalingWindow::Get().Options();
	const uint32_t maxInFlightFrameCount = options.maxProducerInFlightFrames;
	if (!_d3d12Context.InitializeAfterCopyDevice(
		maxInFlightFrameCount,
		D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
		D3D12_COMMAND_LIST_TYPE_COMPUTE,
		true
	)) {
		Logger::Get().Error("初始化 D3D12Context 失败");
		return false;
	}

	_computeContext.Initialize(_d3d12Context);

	_isScRGB = colorInfo.kind != winrt::AdvancedColorKind::StandardDynamicRange;

	_frameSource = std::make_unique<GraphicsCaptureFrameSource>();
	if (!_frameSource->Initialize(_d3d12Context, srcRect, hMonSrc, colorInfo)) {
		Logger::Get().Error("初始化 GraphicsCaptureFrameSource 失败");
		return false;
	}

	{
		const SizeU inputSize = {
			uint32_t(srcRect.right - srcRect.left),
			uint32_t(srcRect.bottom - srcRect.top)
		};

		if (!_effectsDrawer.Initialize(_d3d12Context, colorInfo, inputSize, rendererSize, outputSize)) {
			Logger::Get().Error("EffectsDrawer::Initialize 失败");
			return false;
		}

		assert(outputSize.width <= rendererSize.width && outputSize.height <= rendererSize.height);

		if (!_frameRingBuffer.Initialize(_d3d12Context, outputSize, colorInfo)) {
			Logger::Get().Error("初始化 FrameRingBuffer 失败");
			return false;
		}
	}

	{
		auto& descriptorHeap = _d3d12Context.GetDescriptorHeap();

		// maxInFlightFrameCount + (maxInFlightFrameCount + 1) + (maxInFlightFrameCount + 1)
		HRESULT hr = descriptorHeap.Alloc(maxInFlightFrameCount * 3 + 2, _inputSrvBaseOffset);
		if (FAILED(hr)) {
			Logger::Get().ComError("DescriptorHeap::Alloc 失败", hr);
			return false;
		}

		_outputUavBaseOffset = _inputSrvBaseOffset + maxInFlightFrameCount;
		_outputSrvBaseOffset = _outputUavBaseOffset + maxInFlightFrameCount + 1;

		_CreateInputDescriptors();
		_CreateOutputDescriptors();
	}

	_monitorThread = std::thread(&FrameProducer::_MonitorThreadProc, this);

	if (options.IsBenchmarkMode()) {
		// 不要使用无限大，/fp:fast 下无限大值不可靠
		_stepTimer.Initialize(std::numeric_limits<float>::max(), std::nullopt);
	} else {
		_stepTimer.Initialize(options.minFrameRate, options.maxFrameRate);
	}

	// 最后启动捕获以尽可能推迟显示黄色边框 (Win10) 或禁用圆角 (Win11)
	if (!_frameSource->Start()) {
		Logger::Get().Error("GraphicsCaptureFrameSource::Start 失败");
		return false;
	}

	return true;
}

void FrameProducer::_CreateInputDescriptors() noexcept {
	uint32_t bufferCount = ScalingWindow::Get().Options().maxProducerInFlightFrames;
	ID3D12Device5* device = _d3d12Context.GetDevice();
	auto& descriptorHeap = _d3d12Context.GetDescriptorHeap();
	uint32_t descriptorSize = descriptorHeap.GetDescriptorSize();
	
	CD3DX12_CPU_DESCRIPTOR_HANDLE descriptorCpuHandle(descriptorHeap.GetCpuHandle(_inputSrvBaseOffset));
	
	CD3DX12_SHADER_RESOURCE_VIEW_DESC srvDesc = CD3DX12_SHADER_RESOURCE_VIEW_DESC::Tex2D(
		_isScRGB ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, 1);

	for (uint32_t i = 0; i < bufferCount; ++i) {
		device->CreateShaderResourceView(_frameSource->GetOutput(i), &srvDesc, descriptorCpuHandle);
		descriptorCpuHandle.Offset(descriptorSize);
	}
}

void FrameProducer::_CreateOutputDescriptors() noexcept {
	uint32_t bufferCount = ScalingWindow::Get().Options().maxProducerInFlightFrames + 1;
	ID3D12Device5* device = _d3d12Context.GetDevice();
	auto& descriptorHeap = _d3d12Context.GetDescriptorHeap();
	uint32_t descriptorSize = descriptorHeap.GetDescriptorSize();

	CD3DX12_CPU_DESCRIPTOR_HANDLE uavCpuHandle(descriptorHeap.GetCpuHandle(_outputUavBaseOffset));
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvCpuHandle(descriptorHeap.GetCpuHandle(_outputSrvBaseOffset));

	DXGI_FORMAT format = _isScRGB ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_R8G8B8A8_UNORM;
	CD3DX12_UNORDERED_ACCESS_VIEW_DESC uavDesc = CD3DX12_UNORDERED_ACCESS_VIEW_DESC::Tex2D(format);
	CD3DX12_SHADER_RESOURCE_VIEW_DESC srvDesc = CD3DX12_SHADER_RESOURCE_VIEW_DESC::Tex2D(format, 1);

	for (uint32_t i = 0; i < bufferCount; ++i) {
		ID3D12Resource* resource = _frameRingBuffer.GetBuffer(i);
		device->CreateUnorderedAccessView(resource, nullptr, &uavDesc, uavCpuHandle);
		device->CreateShaderResourceView(resource, &srvDesc, srvCpuHandle);

		uavCpuHandle.Offset(descriptorSize);
		srvCpuHandle.Offset(descriptorSize);
	}
}

HRESULT FrameProducer::_Render() noexcept {
	_stepTimer.PrepareForRender();

	uint32_t frameIndex;
	HRESULT hr = _d3d12Context.BeginFrame(frameIndex, nullptr);
	if (FAILED(hr)) {
		Logger::Get().ComError("D3D12Context::BeginFrame 失败", hr);
		return hr;
	}
	
	ID3D12CommandQueue* commandQueue = _d3d12Context.GetCommandQueue();

	uint32_t frameRingBufferIdx;
	hr = _frameRingBuffer.ProducerBeginFrame(commandQueue, frameRingBufferIdx);
	if (FAILED(hr)) {
		Logger::Get().ComError("FrameRingBuffer::ProducerBeginFrame 失败", hr);
		return hr;
	}

	uint32_t frameSourceOutputIdx;
	hr = _frameSource->Update(frameSourceOutputIdx);
	if (FAILED(hr)) {
		Logger::Get().ComError("GraphicsCaptureFrameSource::Update 失败", hr);
		return hr;
	}

	_computeContext.SetDescriptorHeap(_d3d12Context.GetDescriptorHeap().GetHeap());

	// 输出和输出纹理都处于 COMMON 状态，使用结束后也应处于此状态。inputResource
	// 依赖隐式状态转换。
	ID3D12Resource* inputResource = _frameSource->GetOutput(frameSourceOutputIdx);
	ID3D12Resource* outputResource = _frameRingBuffer.GetBuffer(frameRingBufferIdx);

	_computeContext.InsertTransitionBarrier(
		outputResource,
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS
	);

	hr = _effectsDrawer.Draw(
		_computeContext,
		frameIndex,
		inputResource,
		outputResource,
		_inputSrvBaseOffset + frameSourceOutputIdx,
		_outputUavBaseOffset + frameRingBufferIdx
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("EffectsDrawer::Draw 失败", hr);
		return hr;
	}

	_computeContext.InsertTransitionBarrier(
		outputResource,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_COMMON
	);

	_computeContext.Execute(commandQueue);

	hr = _frameRingBuffer.ProducerEndFrame(commandQueue);
	if (FAILED(hr)) {
		Logger::Get().ComError("FrameRingBuffer::ProducerEndFrame 失败", hr);
		return hr;
	}

	hr = _d3d12Context.EndFrame();
	if (FAILED(hr)) {
		Logger::Get().ComError("D3D12Context::EndFrame 失败", hr);
		return hr;
	}

	return S_OK;
}

void FrameProducer::_MonitorThreadProc() noexcept {
	wil::unique_event_nothrow event;
	if (!event.try_create(wil::EventOptions::None, nullptr)) {
		Logger::Get().Win32Error("创建事件失败");
		return;
	}

	uint64_t frameNumber = 1;

	// 绑定新帧渲染完成时触发的事件
	HRESULT hr = _frameRingBuffer.SetEventOnNewFrame(frameNumber, event.get());
	if (FAILED(hr)) {
		Logger::Get().ComError("FrameRingBuffer::SetEventOnNewFrame 失败", hr);
		return;
	}

	MSG msg;
	while (true) {
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				return;
			}

			DispatchMessage(&msg);
		}

		// 有新帧可用时返回 WAIT_OBJECT_0，有新消息时返回 WAIT_OBJECT_0 + 1
		HANDLE hEvent = event.get();
		if (MsgWaitForMultipleObjectsEx(1, &hEvent, INFINITE, QS_ALLINPUT, MWMO_INPUTAVAILABLE) == WAIT_OBJECT_0) {
			// 通知消费者渲染
			PostMessage(ScalingWindow::Get().Handle(), CommonSharedConstants::WM_FRONTEND_RENDER, 0, 0);

			hr = _frameRingBuffer.SetEventOnNewFrame(frameNumber, event.get());
			if (FAILED(hr)) {
				Logger::Get().ComError("FrameRingBuffer::SetEventOnNewFrame 失败", hr);
				return;
			}
		}
	}
}

bool FrameProducer::_CheckResult(HRESULT hr, std::string_view errorMsg) noexcept {
	assert(_state.load(std::memory_order_relaxed) == ComponentState::NoError);

	if (SUCCEEDED(hr)) {
		return true;
	}

	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
		_state.store(ComponentState::DeviceLost, std::memory_order_relaxed);
	} else {
		_state.store(ComponentState::Error, std::memory_order_relaxed);
	}

	Logger::Get().ComError(errorMsg, hr);
	return false;
}

}
