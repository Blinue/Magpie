#include "pch.h"
#include "DebugInfo.h"
#include "DirectXHelper.h"
#include "EffectDrawerBase.h"
#include "FrameProducer.h"
#include "GraphicsCaptureFrameSource.h"
#include "Logger.h"
#include "Renderer.h"
#include "ScalingWindow.h"
#include "shaders/CopyFrameVS.h"
#include "shaders/CopyFrameVS_SM5.h"
#include "shaders/TextureBlitPS.h"
#include "shaders/TextureBlitPS_SM5.h"
#include "SwapChainPresenter.h"
#include <d3dkmthk.h>
#include <windows.graphics.display.interop.h>

namespace Magpie {

// 如果描述符大小为 32 字节，描述符堆消耗 2MiB 显存
static uint32_t CSU_HEAP_CAPACITY = 65536;
// 目前只有渲染到后缓冲和缩放光标时需要 RTV
static uint32_t RTV_HEAP_CAPACITY = 1024;

Renderer::Renderer() noexcept {}

Renderer::~Renderer() noexcept {
	if (_d3d12Context.GetCommandQueue()) {
		_d3d12Context.WaitForGpu();
	}
}

static void SetGpuPriority() noexcept {
	// 不使用 REALTIME 优先级，它可能造成系统不稳定。
	// 来自 https://github.com/obsproject/obs-studio/blob/16cb051a57bb357fe866252c1360ce2c38e2deec/libobs-d3d11/d3d11-subsystem.cpp#L429
	NTSTATUS status = D3DKMTSetProcessSchedulingPriorityClass(
		GetCurrentProcess(), D3DKMT_SCHEDULINGPRIORITYCLASS_HIGH);
	if (status != STATUS_SUCCESS) {
		Logger::Get().NTError("D3DKMTSetProcessSchedulingPriorityClass 失败", status);
	}
}

ScalingError Renderer::Initialize(
	HWND hwndAttach,
	HMONITOR hMonitor,
	const RECT& srcRect,
	const RECT& rendererRect,
	OverlayOptions& overlayOptions,
	RECT& destRect
) noexcept {
	_hCurMonitor = hMonitor;

	SetGpuPriority();

	const ScalingOptions& options = ScalingWindow::Get().Options();
	if (!_d3d12Context.Initialize(
		options.graphicsCardId,
		options.Is3DGameMode() ? 2 : 6,
		D3D12_COMMAND_QUEUE_PRIORITY_HIGH,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		_csuDescriptorHeap,
		_rtvDescriptorHeap
	)) {
		Logger::Get().Error("初始化 D3D12Context 失败");
		return ScalingError::ScalingFailedGeneral;
	}

	ID3D12Device5* device = _d3d12Context.GetDevice();

#ifdef MP_DEBUG_INFO
	{
		// 禁用动态时钟频率调整
		if (DEBUG_INFO.enableStablePower) {
			HRESULT hr = device->SetStablePowerState(TRUE);
			if (FAILED(hr)) {
				Logger::Get().ComError("SetStablePowerState 失败", hr);
			}
		}

#ifdef _DEBUG
		// 模拟低速 GPU
		winrt::com_ptr<ID3D12DebugDevice1> debugDevice;
		HRESULT hr = device->QueryInterface<ID3D12DebugDevice1>(debugDevice.put());
		if (SUCCEEDED(hr)) {
			D3D12_DEBUG_DEVICE_GPU_SLOWDOWN_PERFORMANCE_FACTOR value = {
				.SlowdownFactor = DEBUG_INFO.gpuSlowDownFactor
			};
			hr = debugDevice->SetDebugParameter(
				D3D12_DEBUG_DEVICE_PARAMETER_GPU_SLOWDOWN_PERFORMANCE_FACTOR, &value, sizeof(value));
			if (FAILED(hr)) {
				Logger::Get().ComError("ID3D12DebugDevice1::SetDebugParameter 失败", hr);
			}
		} else {
			Logger::Get().ComError("获取 ID3D12DebugDevice1 失败", hr);
		}
#endif
	}
#endif

	if (!_csuDescriptorHeap.Initialize(
		device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, CSU_HEAP_CAPACITY)
	) {
		Logger::Get().Error("DescriptorHeap::Initialize 失败");
		return ScalingError::ScalingFailedGeneral;
	}

	if (!_rtvDescriptorHeap.Initialize(
		device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, RTV_HEAP_CAPACITY)
	) {
		Logger::Get().Error("DescriptorHeap::Initialize 失败");
		return ScalingError::ScalingFailedGeneral;
	}

	_graphicsContext.Initialize(_d3d12Context);

	// 失败则回落到使用传统方法获取颜色显示能力
	_TryInitDisplayInfo();

	if (!_UpdateColorInfo()) {
		Logger::Get().Error("_UpdateColorInfo 失败");
		return ScalingError::ScalingFailedGeneral;
	}

	const SizeU rendererSize = {
		uint32_t(rendererRect.right - rendererRect.left),
		uint32_t(rendererRect.bottom - rendererRect.top)
	};

	SizeU outputSize;
	SimpleTask<bool> task;
	_frameProducer.InitializeAsync(
		_d3d12Context, _colorInfo, hMonitor, srcRect, rendererSize, outputSize, task);

	_presenter = std::make_unique<SwapChainPresenter>();
	if (!_presenter->Initialize(_d3d12Context, hwndAttach, rendererSize, _colorInfo)) {
		Logger::Get().Error("SwapChainPresenter::Initialize 失败");
		return ScalingError::ScalingFailedGeneral;
	}

	// 同步点，使生产者线程的修改可见
	if (!task.GetResult(std::memory_order_acquire)) {
		Logger::Get().Error("FrameProducer::InitializeAsync 失败");
		return ScalingError::ScalingFailedGeneral;
	}

	_UpdateOutputRect(outputSize);

	destRect.left = rendererRect.left + (LONG)_outputRect.left;
	destRect.top = rendererRect.top + (LONG)_outputRect.top;
	destRect.right = rendererRect.left + (LONG)_outputRect.right;
	destRect.bottom = rendererRect.top + (LONG)_outputRect.bottom;

	if (!_overlayDrawer.Initialize(_d3d12Context, overlayOptions)) {
		Logger::Get().Error("OverlayDrawer::Initialize 失败");
		return ScalingError::ScalingFailedGeneral;
	}

	if (!_cursorDrawer.Initialize(_d3d12Context, srcRect, rendererRect, destRect, _colorInfo)) {
		Logger::Get().Error("CursorDrawer::Initialize 失败");
		return ScalingError::ScalingFailedGeneral;
	}

	return ScalingError::NoError;
}

ComponentState Renderer::Render(
	HCURSOR hCursor,
	POINT cursorPos,
	bool waitForGpu,
	bool* waitingForFirstFrame
) noexcept {
	assert(!waitingForFirstFrame || !*waitingForFirstFrame);

	if (_state != ComponentState::NoError) {
		return _state;
	}

	_state = _frameProducer.GetState();
	if (_state != ComponentState::NoError) {
		return _state;
	}

	bool needRedraw = false;

	{
		const uint64_t latestProducerFrameNumber = _frameProducer.GetLatestFrameNumber();
		if (latestProducerFrameNumber == 0) {
			if (waitingForFirstFrame && latestProducerFrameNumber == 0) {
				*waitingForFirstFrame = true;
			}
			return _state;
		}

		if (latestProducerFrameNumber != _lastProducerFrameNumber) {
			needRedraw = true;
			_lastProducerFrameNumber = latestProducerFrameNumber;
		}
	}

	{
		bool cursorNeedRedraw = false;
		_cursorDrawer.PrepareForDraw(hCursor, cursorPos, cursorNeedRedraw);
		needRedraw |= cursorNeedRedraw;
	}
	
	if (needRedraw) {
		_CheckResult(_RenderImpl(cursorPos, waitForGpu), "_RenderImpl 失败");
	}

	return _state;
}

void Renderer::OnMonitorChanged(HMONITOR hMonitor) noexcept {
	if (_state != ComponentState::NoError) {
		return;
	}

	_acInfoChangedRevoker.revoke();
	_displayInfo = nullptr;
	_hCurMonitor = hMonitor;

	_TryInitDisplayInfo();
	_CheckResult(_UpdateColorSpace(), "_UpdateColorSpace 失败");
}

void Renderer::OnResizingChanged(bool value) noexcept {
	if (_state != ComponentState::NoError) {
		return;
	}

	if (!_CheckResult(_presenter->OnResizingChanged(value), "SwapChainPresenter::OnResizingChanged 失败")) {
		return;
	}

	_overlayDrawer.OnResizingChanged(value);
}

void Renderer::OnResized(const RECT& rendererRect, RECT& destRect) noexcept {
	if (_state != ComponentState::NoError) {
		return;
	}

	// 确保消费者不再使用环形缓冲区
	if (!_CheckResult(_d3d12Context.WaitForGpu(), "D3D12Context::WaitForGpu 失败")) {
		return;
	}

	const SizeU rendererSize = {
		uint32_t(rendererRect.right - rendererRect.left),
		uint32_t(rendererRect.bottom - rendererRect.top)
	};

	SizeU outputSize;
	SimpleTask<HRESULT> task;
	_frameProducer.OnResizedAsync(rendererSize, outputSize, task);

	if (!_CheckResult(_presenter->OnResized(rendererSize), "SwapChainPresenter::OnResized 失败")) {
		return;
	}

	if (!_CheckResult(task.GetResult(std::memory_order_acquire),
		"FrameProducer::OnResizedAsync 失败")) {
		return;
	}

	_UpdateOutputRect(outputSize);

	destRect.left = rendererRect.left + (LONG)_outputRect.left;
	destRect.top = rendererRect.top + (LONG)_outputRect.top;
	destRect.right = rendererRect.left + (LONG)_outputRect.right;
	destRect.bottom = rendererRect.top + (LONG)_outputRect.bottom;

	_overlayDrawer.OnResized(rendererRect, destRect);
	_cursorDrawer.OnResized(rendererRect, destRect);
}

void Renderer::OnMovingChanged(bool value) noexcept {
	_overlayDrawer.OnMovingChanged(value);
	_cursorDrawer.OnMovingChanged(value);
}

void Renderer::OnMoved(const RECT& rendererRect, RECT& destRect) noexcept {
	destRect.left = rendererRect.left + (LONG)_outputRect.left;
	destRect.top = rendererRect.top + (LONG)_outputRect.top;
	destRect.right = rendererRect.left + (LONG)_outputRect.right;
	destRect.bottom = rendererRect.top + (LONG)_outputRect.bottom;

	_overlayDrawer.OnMoved(rendererRect, destRect);
	_cursorDrawer.OnMoved(rendererRect, destRect);
}

void Renderer::OnCursorVirtualizationChanged(bool value) noexcept {
	_cursorDrawer.OnCursorVirtualizationChanged(value);
}

void Renderer::OnCursorCapturedOnForegroundChanged(bool value) noexcept {
	_overlayDrawer.OnCursorCapturedOnForegroundChanged(value);
}

void Renderer::OnSrcMovingChanged(bool value) noexcept {
	_cursorDrawer.OnSrcMovingChanged(value);
}

void Renderer::OnMsgDisplayChanged() noexcept {
	// winrt::DisplayInformation 可用时已通过事件监听颜色配置变化
	if (_state != ComponentState::NoError || _displayInfo) {
		return;
	}

	_CheckResult(_UpdateColorSpace(), "_UpdateColorSpace 失败");
}

void Renderer::OnCursorVisibilityChanged(bool isVisible, bool onDestory) noexcept {
	_frameProducer.OnCursorVisibilityChanged(isVisible, onDestory);
}

void Renderer::_TryInitDisplayInfo() noexcept {
	// 从 Win11 22H2 开始支持
	winrt::com_ptr<IDisplayInformationStaticsInterop> interop =
		winrt::try_get_activation_factory<winrt::DisplayInformation, IDisplayInformationStaticsInterop>();
	if (!interop) {
		return;
	}
	
	HRESULT hr = interop->GetForMonitor(
		_hCurMonitor, winrt::guid_of<winrt::DisplayInformation>(), winrt::put_abi(_displayInfo));
	if (FAILED(hr)) {
		Logger::Get().ComError("IDisplayInformationStaticsInterop::GetForMonitor 失败", hr);
		return;
	}

	_acInfoChangedRevoker = _displayInfo.AdvancedColorInfoChanged(
		winrt::auto_revoke,
		[this](winrt::DisplayInformation const&, winrt::IInspectable const&) {
			_CheckResult(_UpdateColorSpace(), "_UpdateColorSpace 失败");
		}
	);
}

static float GetSDRWhiteLevel(std::wstring_view monitorName) noexcept {
	UINT32 pathCount = 0, modeCount = 0;
	if (GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &pathCount, &modeCount) != ERROR_SUCCESS) {
		return 1.0f;
	}

	std::vector<DISPLAYCONFIG_PATH_INFO> paths(pathCount);
	std::vector<DISPLAYCONFIG_MODE_INFO> modes(modeCount);
	if (QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &pathCount, paths.data(), &modeCount, modes.data(), nullptr) != ERROR_SUCCESS) {
		return 1.0f;
	}

	for (const DISPLAYCONFIG_PATH_INFO& path : paths) {
		DISPLAYCONFIG_SOURCE_DEVICE_NAME sourceName = {
			.header = {
				.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME,
				.size = sizeof(sourceName),
				.adapterId = path.sourceInfo.adapterId,
				.id = path.sourceInfo.id
			}
		};
		if (DisplayConfigGetDeviceInfo(&sourceName.header) != ERROR_SUCCESS) {
			continue;
		}

		if (monitorName == sourceName.viewGdiDeviceName) {
			DISPLAYCONFIG_SDR_WHITE_LEVEL sdr = {
				.header = {
					.type = DISPLAYCONFIG_DEVICE_INFO_GET_SDR_WHITE_LEVEL,
					.size = sizeof(sdr),
					.adapterId = path.targetInfo.adapterId,
					.id = path.targetInfo.id
				}
			};
			if (DisplayConfigGetDeviceInfo(&sdr.header) == ERROR_SUCCESS) {
				return sdr.SDRWhiteLevel / 1000.0f;
			} else {
				return 1.0f;
			}
		}
	}

	return 1.0f;
}

bool Renderer::_UpdateColorInfo() noexcept {
	if (_displayInfo) {
		winrt::AdvancedColorInfo acInfo = _displayInfo.GetAdvancedColorInfo();

		_colorInfo.kind = acInfo.CurrentAdvancedColorKind();
		if (_colorInfo.kind == winrt::AdvancedColorKind::HighDynamicRange) {
			_colorInfo.maxLuminance = acInfo.MaxLuminanceInNits() / SCENE_REFERRED_SDR_WHITE_LEVEL;
			_colorInfo.sdrWhiteLevel = acInfo.SdrWhiteLevelInNits() / SCENE_REFERRED_SDR_WHITE_LEVEL;
		} else {
			_colorInfo.maxLuminance = 1.0f;
			_colorInfo.sdrWhiteLevel = 1.0f;
		}

		return true;
	}

	IDXGIFactory7* dxgiFactory = _d3d12Context.GetDXGIFactoryForEnumingAdapters();
	if (!dxgiFactory) {
		return false;
	}

	winrt::com_ptr<IDXGIAdapter1> adapter;
	winrt::com_ptr<IDXGIOutput> output;
	for (UINT adapterIdx = 0;
		SUCCEEDED(dxgiFactory->EnumAdapters1(adapterIdx, adapter.put()));
		++adapterIdx
	) {
		for (UINT outputIdx = 0;
			SUCCEEDED(adapter->EnumOutputs(outputIdx, output.put()));
			++outputIdx
		) {
			DXGI_OUTPUT_DESC1 desc;
			if (SUCCEEDED(output.try_as<IDXGIOutput6>()->GetDesc1(&desc))) {
				if (desc.Monitor == _hCurMonitor) {
					// DXGI 将 WCG 视为 SDR
					if (desc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020) {
						_colorInfo.kind = winrt::AdvancedColorKind::HighDynamicRange;
						_colorInfo.maxLuminance = desc.MaxLuminance / SCENE_REFERRED_SDR_WHITE_LEVEL;
						_colorInfo.sdrWhiteLevel = GetSDRWhiteLevel(desc.DeviceName);
					} else {
						_colorInfo.kind = winrt::AdvancedColorKind::StandardDynamicRange;
						_colorInfo.maxLuminance = 1.0f;
						_colorInfo.sdrWhiteLevel = 1.0f;
					}

					return true;
				}
			}
		}
	}

	// 未找到视为 SDR
	_colorInfo.kind = winrt::AdvancedColorKind::StandardDynamicRange;
	_colorInfo.maxLuminance = 1.0f;
	_colorInfo.sdrWhiteLevel = 1.0f;
	return true;
}

HRESULT Renderer::_UpdateColorSpace() noexcept {
	ColorInfo oldColorInfo = _colorInfo;
	if (!_UpdateColorInfo()) {
		return E_FAIL;
	}

	if (oldColorInfo == _colorInfo) {
		return S_OK;
	}

	// 确保消费者不再使用环形缓冲区
	HRESULT hr = _d3d12Context.WaitForGpu();
	if (FAILED(hr)) {
		Logger::Get().ComError("D3D12Context::WaitForGpu 失败", hr);
		return hr;
	}

	SimpleTask<HRESULT> task;
	_frameProducer.OnColorInfoChangedAsync(_colorInfo, task);

	_cursorDrawer.OnColorInfoChanged(_colorInfo);

	hr = _presenter->OnColorInfoChanged(_colorInfo);
	if (FAILED(hr)) {
		Logger::Get().ComError("SwapChainPresenter::OnColorInfoChanged 失败", hr);
		return hr;
	}

	hr = task.GetResult();
	if (FAILED(hr)) {
		Logger::Get().ComError("FrameProducer::OnColorInfoChangedAsync 失败", hr);
		return hr;
	}

	// 生产者渲染完成会通知消费者渲染新帧
	return S_OK;
}

HRESULT Renderer::_RenderImpl(POINT cursorPos, bool waitForGpu) noexcept {
	// 处于 COMMON 状态，依赖隐式状态转换
	ID3D12Resource* curFrame;
	uint32_t curFrameSrvOffset;
	uint64_t completedFenceValue;
	uint64_t fenceValueToSignal;
	if (!_frameProducer.ConsumerBeginFrame(
		curFrame, curFrameSrvOffset, completedFenceValue, fenceValueToSignal)) {
		// 不应出现第一帧未完成的情况
		assert(false);
		return S_OK;
	}

	// SwapChain::BeginFrame 和 D3D12Context::BeginFrame 无顺序要求，不过
	// 前者通常等待时间更久，将它放在前面可以减少等待次数。
	ID3D12Resource* backBuffer;
	uint32_t rtvOffset, rawRtvOffset;
	_presenter->BeginFrame(&backBuffer, rtvOffset, rawRtvOffset);

	{
		bool isSrgb = _colorInfo.kind == winrt::AdvancedColorKind::StandardDynamicRange;
		winrt::com_ptr<ID3D12PipelineState>& pso = isSrgb ? _copyFrameSrgbPSO : _copyFramePSO;
		if (!pso) {
			HRESULT hr = _CreateCopyFramePSO(isSrgb, pso);
			if (FAILED(hr)) {
				Logger::Get().ComError("_CreateCopyFramePSO 失败", hr);
				return hr;
			}
		}

		uint32_t frameIndex;
		HRESULT hr = _d3d12Context.BeginFrame(frameIndex, pso.get());
		if (FAILED(hr)) {
			Logger::Get().ComError("D3D12Context::BeginFrame 失败", hr);
			return hr;
		}
	}
	
	_graphicsContext.SetDescriptorHeap(_csuDescriptorHeap.GetHeap());

	_graphicsContext.IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	_graphicsContext.InsertTransitionBarrier(
		backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	_graphicsContext.SetRootSignature(_copyRootSignature.get());

	const SizeU rendererSize = _presenter->GetSize();

	{
		float outputWidth = float(_outputRect.right - _outputRect.left);
		float outputHeight = float(_outputRect.bottom - _outputRect.top);
		// 这些参数将输出区域 uv 变换为 0~1
		float constants[] = {
			rendererSize.width / outputWidth,
			rendererSize.height / outputHeight,
			_outputRect.left / -outputWidth,
			_outputRect.top / -outputHeight
		};
		_graphicsContext.SetRoot32BitConstants(0, (uint32_t)std::size(constants), constants);
	}

	_graphicsContext.SetRootDescriptorTable(1, curFrameSrvOffset);

	_graphicsContext.RSSetViewportAndScissorRect(CD3DX12_RECT(
		0, 0, (LONG)rendererSize.width, (LONG)rendererSize.height));
	
	_graphicsContext.OMSetRenderTarget(rtvOffset);

	_graphicsContext.Draw(3);

	_overlayDrawer.Draw(_graphicsContext, cursorPos, _frameProducer.GetFPS());

	// 为了和 OS 保持一致，SDR 下绘制光标时在 sRGB 空间中混合
	if (_colorInfo.kind == winrt::AdvancedColorKind::StandardDynamicRange) {
		_graphicsContext.OMSetRenderTarget(rawRtvOffset);
	}

	HRESULT hr = _cursorDrawer.Draw(_graphicsContext, completedFenceValue, fenceValueToSignal,
		curFrameSrvOffset, nullptr);
	if (FAILED(hr)) {
		Logger::Get().ComError("CursorDrawer::Draw 失败", hr);
		return hr;
	}

	_graphicsContext.InsertTransitionBarrier(
		backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	ID3D12CommandQueue* commandQueue = _d3d12Context.GetCommandQueue();

	hr = _graphicsContext.Execute(commandQueue);
	if (FAILED(hr)) {
		Logger::Get().ComError("CommandContext::Execute 失败", hr);
		return hr;
	}

	hr =_frameProducer.ConsumerEndFrame(commandQueue, fenceValueToSignal);
	if (FAILED(hr)) {
		Logger::Get().ComError("FrameProducer::ConsumerEndFrame 失败", hr);
		return hr;
	}

	hr = _presenter->EndFrame(waitForGpu);
	if (FAILED(hr)) {
		Logger::Get().ComError("SwapChainPresenter::EndFrame 失败", hr);
		return hr;
	}

	// D3D12Context::EndFrame 必须在 SwapChain::EndFrame 之后
	hr = _d3d12Context.EndFrame();
	if (FAILED(hr)) {
		Logger::Get().ComError("D3D12Context::EndFrame 失败", hr);
		return hr;
	}

	return S_OK;
}

void Renderer::_UpdateOutputRect(SizeU outputSize) noexcept {
	// TODO: 窗口模式缩放始终填充画面
	const SizeU rendererSize = _presenter->GetSize();
	OutputAlignment alignment = ScalingWindow::Get().Options().outputAlignment;

	using enum OutputAlignment;

	if (alignment == LeftTop || alignment == Left || alignment == LeftBottom) {
		_outputRect.left = 0;
		_outputRect.right = outputSize.width;
	} else if (alignment == Top || alignment == Center || alignment == Bottom) {
		_outputRect.left = (rendererSize.width - outputSize.width) / 2;
		_outputRect.right = _outputRect.left + outputSize.width;
	} else {
		_outputRect.left = rendererSize.width - outputSize.width;
		_outputRect.right = rendererSize.width;
	}

	if (alignment == LeftTop || alignment == Top || alignment == RightTop) {
		_outputRect.top = 0;
		_outputRect.bottom = outputSize.height;
	} else if (alignment == Left || alignment == Center || alignment == Right) {
		_outputRect.top = (rendererSize.height - outputSize.height) / 2;
		_outputRect.bottom = _outputRect.top + outputSize.height;
	} else {
		_outputRect.top = rendererSize.height - outputSize.height;
		_outputRect.bottom = rendererSize.height;
	}

	assert(_outputRect.left + outputSize.width == _outputRect.right);
	assert(_outputRect.top + outputSize.height == _outputRect.bottom);
}

bool Renderer::_CheckResult(bool success, std::string_view errorMsg) noexcept {
	assert(_state == ComponentState::NoError);

	if (!success) {
		_state = ComponentState::Error;
		Logger::Get().Error(errorMsg);
	}
	return success;
}

bool Renderer::_CheckResult(HRESULT hr, std::string_view errorMsg) noexcept {
	assert(_state == ComponentState::NoError);

	if (SUCCEEDED(hr)) {
		return true;
	}

	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
		_state = ComponentState::DeviceLost;
	} else {
		_state = ComponentState::Error;
	}

	Logger::Get().ComError(errorMsg, hr);
	return false;
}

HRESULT Renderer::_CreateCopyFramePSO(bool isSrgb, winrt::com_ptr<ID3D12PipelineState>& result) noexcept {
	ID3D12Device5* device = _d3d12Context.GetDevice();

	if (!_copyRootSignature) {
		CD3DX12_DESCRIPTOR_RANGE1 srvRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0,
			D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);
		D3D12_ROOT_PARAMETER1 rootParams[] = {
			{
				.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
				.Constants = {
					.Num32BitValues = 4
				},
				.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX
			},
			{
				.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
				.DescriptorTable = {
					.NumDescriptorRanges = 1,
					.pDescriptorRanges = &srvRange
				},
				.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL
			}
		};
		D3D12_STATIC_SAMPLER_DESC samplerDesc = {
			.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT,
			.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
			// 边界外使用黑色填充
			.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK,
			.ShaderRegister = 0
		};
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc(
			(UINT)std::size(rootParams), rootParams, 1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_NONE);

		winrt::com_ptr<ID3DBlob> signature;
		HRESULT hr = D3DX12SerializeVersionedRootSignature(
			&rootSignatureDesc, _d3d12Context.GetRootSignatureVersion(), signature.put(), nullptr);
		if (FAILED(hr)) {
			Logger::Get().ComError("D3DX12SerializeVersionedRootSignature 失败", hr);
			return hr;
		}

		hr = device->CreateRootSignature(
			0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&_copyRootSignature));
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateRootSignature 失败", hr);
			return hr;
		}
	}
	
	bool isSM6Supported = _d3d12Context.GetShaderModel() >= D3D_SHADER_MODEL_6_0;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {
		.pRootSignature = _copyRootSignature.get(),
		.VS = DirectXHelper::SelectShader(isSM6Supported, CopyFrameVS, CopyFrameVS_SM5),
		.PS = DirectXHelper::SelectShader(isSM6Supported, TextureBlitPS, TextureBlitPS_SM5),
		.BlendState = {
			.RenderTarget = {{ .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL }}
		},
		.SampleMask = UINT_MAX,
		.RasterizerState = {
			.FillMode = D3D12_FILL_MODE_SOLID,
			.CullMode = D3D12_CULL_MODE_NONE
		},
		.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
		.NumRenderTargets = 1,
		.RTVFormats = { isSrgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R16G16B16A16_FLOAT },
		.SampleDesc = { .Count = 1 }
	};
	HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&result));
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateGraphicsPipelineState 失败", hr);
		return hr;
	}

	return S_OK;
}

}
