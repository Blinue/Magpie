#include "pch.h"
#include "Renderer.h"
#include "DeviceResources.h"
#include "ScalingOptions.h"
#include "Logger.h"
#include "Win32Helper.h"
#include "EffectDrawer.h"
#include "StrHelper.h"
#include "EffectCompiler.h"
#include "GraphicsCaptureFrameSource.h"
#include "DesktopDuplicationFrameSource.h"
#include "GDIFrameSource.h"
#include "DwmSharedSurfaceFrameSource.h"
#include "DirectXHelper.h"
#include "ScalingWindow.h"
#include "OverlayDrawer.h"
#include "CursorManager.h"
#include "EffectsProfiler.h"
#include "CommonSharedConstants.h"
#ifdef MP_USE_COMPSWAPCHAIN
#include "CompSwapchainPresenter.h"
#else
#include "AdaptivePresenter.h"
#endif
#include "ScreenshotHelper.h"
#include "TextureHelper.h"
#include <dispatcherqueue.h>
#include <d3dkmthk.h>

namespace Magpie {

// 大多数时候会在最后添加 Bicubic 来降采样或升采样，因此缓存在内存中
static EffectDesc bicubicDesc;

Renderer::Renderer() noexcept {}

Renderer::~Renderer() noexcept {
	_hKeyboardHook.reset();

	if (_backendThread.joinable()) {
		const HANDLE hThread = _backendThread.native_handle();

		if (!wil::handle_wait(hThread, 0)) {
			const DWORD threadId = GetThreadId(_backendThread.native_handle());

			// 持续尝试直到 _backendThread 创建了消息队列
			while (!PostThreadMessage(threadId, WM_QUIT, 0, 0)) {
				if (wil::handle_wait(hThread, 1)) {
					break;
				}
			}
		}
		
		_backendThread.join();
	}
}

static void LogAdapter(IDXGIAdapter4* adapter) noexcept {
	DXGI_ADAPTER_DESC1 desc;
	adapter->GetDesc1(&desc);

	Logger::Get().Info(fmt::format("当前图形适配器: \n\tVendorId: {:#x}\n\tDeviceId: {:#x}\n\tDescription: {}",
		desc.VendorId, desc.DeviceId, StrHelper::UTF16ToUTF8(desc.Description)));
}

static void SetGpuPriority() noexcept {
	// 来自 https://github.com/obsproject/obs-studio/blob/16cb051a57bb357fe866252c1360ce2c38e2deec/libobs-d3d11/d3d11-subsystem.cpp#L429
	// 不使用 REALTIME 优先级，它会造成系统不稳定，而且可能会导致源窗口卡顿。
	// OBS 还调用了 SetGPUThreadPriority，但这个接口似乎无用。
	NTSTATUS status = D3DKMTSetProcessSchedulingPriorityClass(
		GetCurrentProcess(), D3DKMT_SCHEDULINGPRIORITYCLASS_HIGH);
	if (status != STATUS_SUCCESS) {
		Logger::Get().NTError("D3DKMTSetProcessSchedulingPriorityClass 失败", status);
	}
}

ScalingError Renderer::Initialize(HWND hwndAttach, OverlayOptions& overlayOptions) noexcept {
	_backendThread = std::thread(&Renderer::_BackendThreadProc, this);

	if (!_frontendResources.Initialize(true)) {
		Logger::Get().Error("初始化前端资源失败");
		return ScalingError::ScalingFailedGeneral;
	}

	LogAdapter(_frontendResources.GetGraphicsAdapter());

	// 每次创建 D3D 设备后尝试提高 GPU 优先级，OBS 也是这么做的
	SetGpuPriority();

#ifdef MP_USE_COMPSWAPCHAIN
	_presenter = std::make_unique<CompSwapchainPresenter>();
	if (!_presenter->Initialize(hwndAttach, _frontendResources)) {
		Logger::Get().Error("初始化 CompSwapchainPresenter 失败");
#else
	_presenter = std::make_unique<AdaptivePresenter>();
	if (!_presenter->Initialize(hwndAttach, _frontendResources)) {
		Logger::Get().Error("初始化 AdaptivePresenter 失败");
#endif
		return ScalingError::ScalingFailedGeneral;
	}

	// 等待后端初始化完成
	_sharedTextureHandle.wait(NULL, std::memory_order_relaxed);
	const HANDLE sharedTextureHandle = _sharedTextureHandle.load(std::memory_order_acquire);
	if (sharedTextureHandle == INVALID_HANDLE_VALUE) {
		Logger::Get().Error("后端初始化失败");
		// 一般的错误不会设置 _backendInitError
		return _backendInitError == ScalingError::NoError ? ScalingError::ScalingFailedGeneral : _backendInitError;
	}

	// 获取共享纹理
	HRESULT hr = _frontendResources.GetD3DDevice()->OpenSharedResource(
		sharedTextureHandle, IID_PPV_ARGS(_frontendSharedTexture.put()));
	if (FAILED(hr)) {
		Logger::Get().ComError("OpenSharedResource 失败", hr);
		return ScalingError::ScalingFailedGeneral;
	}

	_frontendSharedTextureMutex = _frontendSharedTexture.try_as<IDXGIKeyedMutex>();

	_UpdateDestRect();

	Logger::Get().Info(fmt::format("目标矩形: {},{},{},{} ({}x{})",
		_destRect.left, _destRect.top, _destRect.right, _destRect.bottom,
		_destRect.right - _destRect.left, _destRect.bottom - _destRect.top));

	if (!_cursorDrawer.Initialize(_frontendResources)) {
		Logger::Get().ComError("初始化 CursorDrawer 失败", hr);
		return ScalingError::ScalingFailedGeneral;
	}

	if (!_overlayDrawer.Initialize(_frontendResources, overlayOptions)) {
		Logger::Get().Error("初始化 OverlayDrawer 失败");
		return ScalingError::ScalingFailedGeneral;
	}

	if (!ScalingWindow::Get().Options().Is3DGameMode()) {
		_overlayDrawer.ToolbarState(ScalingWindow::Get().Options().initialToolbarState);
	}

	_hKeyboardHook.reset(SetWindowsHookEx(WH_KEYBOARD_LL, _LowLevelKeyboardHook, NULL, 0));
	if (!_hKeyboardHook) {
		Logger::Get().Win32Warn("SetWindowsHookEx 失败");
	}

	return ScalingError::NoError;
}

void Renderer::OnCursorVisibilityChanged(bool isVisible, bool onDestory) {
	_backendThreadDispatcher.TryEnqueue([this, isVisible, onDestory]() {
		if (_frameSource) {
			_frameSource->OnCursorVisibilityChanged(isVisible, onDestory);
		}
	});
}

void Renderer::MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
	if (!_overlayDrawer.AnyVisibleWindow()) {
		return;
	}

	_overlayDrawer.MessageHandler(msg, wParam, lParam);

	// 有些鼠标操作需要渲染 ImGui 多次，见 https://github.com/ocornut/imgui/issues/2268
	if (msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MOUSEWHEEL ||
		msg == WM_MOUSEHWHEEL || msg == WM_LBUTTONUP || msg == WM_RBUTTONUP) {
		_FrontendRender();
	}
}

void Renderer::StartProfile() noexcept {
	_backendThreadDispatcher.TryEnqueue([this] {
		uint32_t passCount = 0;
		for (const EffectDesc* desc : _activeEffectDescs) {
			passCount += (uint32_t)desc->passes.size();
		}
		_effectsProfiler.Start(_backendResources.GetD3DDevice(), passCount);
	});
}

void Renderer::StopProfile() noexcept {
	_backendThreadDispatcher.TryEnqueue([this] {
		_effectsProfiler.Stop();
	});
}

winrt::fire_and_forget Renderer::TakeScreenshot(
	uint32_t effectIdx,
	uint32_t passIdx,
	uint32_t outputIdx
) noexcept {
	assert(effectIdx < _activeEffectDescs.size());

	if (!co_await _TakeScreenshotImpl(effectIdx, passIdx, outputIdx)) {
		Logger::Get().Error("_TakeScreenshotImpl 失败");
		ScalingWindow::Get().ShowToast(
			ScalingWindow::Get().GetLocalizedString(L"Message_ScreenshotFailed"));
	}
}

void Renderer::_FrontendRender(bool waitForRenderComplete) noexcept {
	winrt::com_ptr<ID3D11Texture2D> frameTex;
	winrt::com_ptr<ID3D11RenderTargetView> frameRtv;
	POINT drawOffset;
	if (!_presenter->BeginFrame(frameTex, frameRtv, drawOffset)) {
		return;
	}

	ID3D11DeviceContext4* d3dDC = _frontendResources.GetD3DDC();
	d3dDC->ClearState();

	// 所有渲染都使用三角形带拓扑
	d3dDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	const RECT& rendererRect = ScalingWindow::Get().RendererRect();
	if (_destRect != rendererRect) {
		// 存在黑边时应以黑色填充背景。使用交换链呈现时需要这个操作，因为我们指定了 
		// DXGI_SWAP_EFFECT_FLIP_DISCARD，同时也是为了和 RTSS 兼容。使用 DirectComposition
		// 呈现时也需要这个操作，因为必须渲染所有像素。
		// 
		// 当渲染到 IDCompositionSurface 上时这个调用并不符合标准，文档说不应该在更新矩形外绘
		// 制。不过 Chromium 也是这么做的，而且声称这个操作只会影响更新矩形中的像素。见
		// https://github.com/chromium/chromium/blob/3653c48c3dc9ca9004f241a79238a1b3e0d0c633/gpu/command_buffer/service/shared_image/dcomp_surface_image_backing.cc#L63
		static constexpr FLOAT BLACK[4] = { 0.0f,0.0f,0.0f,1.0f };
		d3dDC->ClearRenderTargetView(frameRtv.get(), BLACK);
	}

	_lastAccessMutexKey = ++_sharedTextureMutexKey;
	HRESULT hr = _frontendSharedTextureMutex->AcquireSync(_lastAccessMutexKey - 1, INFINITE);
	if (FAILED(hr)) {
		Logger::Get().ComError("AcquireSync 失败", hr);
		return;
	}

	{
		D3D11_TEXTURE2D_DESC desc;
		frameTex->GetDesc(&desc);
		if ((LONG)desc.Width == _destRect.right - _destRect.left
			&& (LONG)desc.Height == _destRect.bottom - _destRect.top) {
			d3dDC->CopyResource(frameTex.get(), _frontendSharedTexture.get());
		} else {
			d3dDC->CopySubresourceRegion(
				frameTex.get(),
				0,
				drawOffset.x + _destRect.left - rendererRect.left,
				drawOffset.y + _destRect.top - rendererRect.top,
				0,
				_frontendSharedTexture.get(),
				0,
				nullptr
			);
		}
	}

	_frontendSharedTextureMutex->ReleaseSync(_lastAccessMutexKey);

	// 叠加层和光标都绘制到 back buffer
	{
		ID3D11RenderTargetView* t = frameRtv.get();
		d3dDC->OMSetRenderTargets(1, &t, nullptr);
	}

	// 绘制叠加层。ImGui 至少渲染两遍，否则经常有布局错误
	_overlayDrawer.Draw(2, _stepTimer.FPS(), _effectsProfiler.GetTimings(), drawOffset);

	// 绘制光标
	_cursorDrawer.Draw(frameTex.get(), drawOffset);
	
	_presenter->EndFrame(waitForRenderComplete);
}

bool Renderer::Render(bool force, bool waitForRenderComplete) noexcept {
	if (!force && _lastAccessMutexKey == _sharedTextureMutexKey.load(std::memory_order_relaxed)) {
		if (_lastAccessMutexKey == 0) {
			// 第一帧尚未完成
			return false;
		}

		if (!_cursorDrawer.NeedRedraw() && !_overlayDrawer.NeedRedraw(_stepTimer.FPS())) {
			return false;
		}
	}

	_FrontendRender(waitForRenderComplete);
	return true;
}

bool Renderer::OnResize() noexcept {
	if (!_presenter->OnResize()) {
		Logger::Get().Error("更改呈现器尺寸失败");
		return false;
	}

	_sharedTextureHandle.store(NULL, std::memory_order_relaxed);

	_backendThreadDispatcher.TryEnqueue([this]() {
		ID3D11Texture2D* outputTexture = _ResizeEffects();
		if (!outputTexture) {
			Logger::Get().Win32Error("_ResizeEffects 失败");
			_sharedTextureHandle.store(INVALID_HANDLE_VALUE, std::memory_order_relaxed);
			_sharedTextureHandle.notify_one();
			return;
		}

		HANDLE sharedHandle = _CreateSharedTexture(outputTexture);
		if (!sharedHandle) {
			Logger::Get().Win32Error("_CreateSharedTexture 失败");
			_sharedTextureHandle.store(INVALID_HANDLE_VALUE, std::memory_order_relaxed);
			_sharedTextureHandle.notify_one();
			return;
		}

		_sharedTextureMutexKey.store(0, std::memory_order_relaxed);

		// 渲染完成再通知前端防止黑屏。前端会自动执行渲染，因此无需发送 WM_FRONTEND_RENDER
		_BackendRender(outputTexture);

		_sharedTextureHandle.store(sharedHandle, std::memory_order_release);
		_sharedTextureHandle.notify_one();
	});

	// 等待后端更改分辨率和渲染
	_sharedTextureHandle.wait(NULL, std::memory_order_relaxed);
	// 将三个成员同步到前端线程
	const HANDLE sharedTextureHandle = _sharedTextureHandle.load(std::memory_order_acquire);
	if (sharedTextureHandle == INVALID_HANDLE_VALUE) {
		return false;
	}

	// 获取共享纹理
	HRESULT hr = _frontendResources.GetD3DDevice()->OpenSharedResource(
		sharedTextureHandle, IID_PPV_ARGS(_frontendSharedTexture.put()));
	if (FAILED(hr)) {
		Logger::Get().ComError("OpenSharedResource 失败", hr);
		return false;
	}

	_frontendSharedTextureMutex = _frontendSharedTexture.try_as<IDXGIKeyedMutex>();
	// 必须重置 _lastAccessMutexKey，确保不会和 _sharedTextureMutexKey 刚巧相同导致接下来的渲染被跳过
	_lastAccessMutexKey = 0;

	_UpdateDestRect();
	return true;
}

void Renderer::OnEndResize() noexcept {
	bool shouldRedraw = false;
	_presenter->OnEndResize(shouldRedraw);

	if (shouldRedraw) {
		_FrontendRender();
	}
}

void Renderer::OnMove() noexcept {
	_UpdateDestRect();
}

void Renderer::OnSrcStartMove() noexcept {
	_presenter->OnSrcStartMove();
}

void Renderer::OnSrcEndMove() noexcept {
	bool shouldRedraw = false;
	_presenter->OnSrcEndMove(shouldRedraw);

	if (shouldRedraw) {
		_FrontendRender();
	}
}

void Renderer::ToggleToolbarState() noexcept {
	const ScalingWindow& scalingWindow = ScalingWindow::Get();

	if (scalingWindow.Options().Is3DGameMode()) {
		scalingWindow.ShowToast(scalingWindow.GetLocalizedString(L"Message_ToolbarIn3DGameMode"));
		return;
	}

	const ToolbarState newState = ToolbarState(
		((uint32_t)_overlayDrawer.ToolbarState() + 1) % (uint32_t)ToolbarState::COUNT);
	_overlayDrawer.ToolbarState(newState);

	// 显示状态切换消息
	const wchar_t* stateResName = nullptr;
	if (newState == ToolbarState::Off) {
		stateResName = L"Home_Toolbar_InitialState_Off/Content";
	} else if (newState == ToolbarState::AlwaysShow) {
		stateResName = L"Home_Toolbar_InitialState_AlwaysShow/Content";
	} else {
		stateResName = L"Home_Toolbar_InitialState_AutoHide/Content";
	}

	winrt::hstring newStateMsg = scalingWindow.GetLocalizedString(L"Message_ToolbarNewState");
	scalingWindow.ShowToast(fmt::format(
		fmt::runtime(std::wstring_view(newStateMsg)),
		std::wstring_view(scalingWindow.GetLocalizedString(stateResName))
	));

	// 立即渲染一帧
	_FrontendRender();
}

const RECT& Renderer::SrcRect() const noexcept {
	return ScalingWindow::Get().SrcInfo().SrcRect();
}

bool Renderer::_InitFrameSource() noexcept {
	switch (ScalingWindow::Get().Options().captureMethod) {
	case CaptureMethod::GraphicsCapture:
		_frameSource = std::make_unique<GraphicsCaptureFrameSource>();
		break;
	case CaptureMethod::DesktopDuplication:
		_frameSource = std::make_unique<DesktopDuplicationFrameSource>();
		break;
	case CaptureMethod::GDI:
		_frameSource = std::make_unique<GDIFrameSource>();
		break;
	case CaptureMethod::DwmSharedSurface:
		_frameSource = std::make_unique<DwmSharedSurfaceFrameSource>();
		break;
	default:
		Logger::Get().Error("未知的捕获模式");
		return false;
	}

	Logger::Get().Info(StrHelper::Concat("当前捕获模式: ", _frameSource->Name()));

	if (!_frameSource->Initialize(_backendResources, _backendDescriptorStore)) {
		Logger::Get().Error("初始化 FrameSource 失败");
		_backendInitError = ScalingError::CaptureFailed;
		return false;
	}

	// 由于 DPI 缩放，捕获尺寸和边界矩形尺寸不一定相同
	D3D11_TEXTURE2D_DESC desc;
	_frameSource->GetOutput()->GetDesc(&desc);
	Logger::Get().Info(fmt::format("捕获尺寸: {}x{}", desc.Width, desc.Height));

	return true;
}

static std::optional<EffectDesc> CompileEffect(
	const EffectOption& effectOption,
	bool noFP16,
	bool forceInlineParams = false
) noexcept {
	// 指定效果名
	EffectDesc result{ .name = effectOption.name };

	uint32_t compileFlag = 0;
	const ScalingOptions& scalingOptions = ScalingWindow::Get().Options();
	if (scalingOptions.IsEffectCacheDisabled()) {
		compileFlag |= EffectCompilerFlags::NoCache;
	}
	if (scalingOptions.IsSaveEffectSources()) {
		compileFlag |= EffectCompilerFlags::SaveSources;
	}
	if (scalingOptions.IsWarningsAreErrors()) {
		compileFlag |= EffectCompilerFlags::WarningsAreErrors;
	}
	if (scalingOptions.IsInlineParams() || forceInlineParams) {
		compileFlag |= EffectCompilerFlags::InlineParams;
	}
	if (noFP16) {
		compileFlag |= EffectCompilerFlags::NoFP16;
	}

	bool success = true;
	uint32_t duration = Measure([&]() {
		success = !EffectCompiler::Compile(result, compileFlag, &effectOption.parameters);
	});

	if (success) {
		Logger::Get().Info(fmt::format("编译 {}.hlsl 用时 {} 毫秒",
			effectOption.name, duration / 1000.0f));
		return result;
	} else {
		Logger::Get().Error(StrHelper::Concat("编译 ",
			effectOption.name, ".hlsl 失败"));
		return std::nullopt;
	}
}

ID3D11Texture2D* Renderer::_BuildEffects() noexcept {
	const ScalingOptions& options = ScalingWindow::Get().Options();
	const bool noFP16 = !_backendResources.IsFP16Supported() || options.IsFP16Disabled();

	const std::vector<EffectOption>& effects = options.effects;
	assert(!effects.empty());
	const uint32_t effectCount = (uint32_t)effects.size();

	// 并行编译所有效果
	_effectDescs.resize(effects.size());
	bool anyFailure = false;
	wil::srwlock writeLock;
	
	int duration = Measure([&]() {
		Win32Helper::RunParallel([&](uint32_t id) {
			std::optional<EffectDesc> desc = CompileEffect(effects[id], noFP16);

			auto lk = writeLock.lock_exclusive();
			if (desc) {
				_effectDescs[id] = std::move(*desc);
			} else {
				anyFailure = true;
			}
		}, effectCount);
	});

	if (anyFailure) {
		return nullptr;
	}

	if (effectCount > 1) {
		Logger::Get().Info(fmt::format("编译着色器总计用时 {} 毫秒", duration / 1000.0f));
	}

	_effectDrawers.resize(effectCount);

	ID3D11Texture2D* inOutTexture = _frameSource->GetOutput();
	for (uint32_t i = 0; i < effectCount; ++i) {
		if (!_effectDrawers[i].Initialize(
			_effectDescs[i],
			effects[i],
			_backendResources,
			_backendDescriptorStore,
			&inOutTexture
		)) {
			Logger::Get().Error(fmt::format("初始化效果#{} ({}) 失败", i, effects[i].name));
			return nullptr;
		}

		// 释放 CSO 内存，不再需要它们
		for (EffectPassDesc& passDesc : _effectDescs[i].passes) {
			passDesc.cso = nullptr;
		}
	}
	
	if (_ShouldAppendBicubic(inOutTexture)) {
		if (!_AppendBicubic(&inOutTexture)) {
			Logger::Get().Error("_AppendBicubic 失败");
			return nullptr;
		}
	}

	_UpdateActiveEffectDescs();

	// 初始化所有效果共用的动态常量缓冲区
	for (const EffectDesc& effectDesc : _effectDescs) {
		if (effectDesc.flags & EffectFlags::UseDynamic) {
			D3D11_BUFFER_DESC bd{
				.ByteWidth = 16,	// 只用 4 个字节
				.Usage = D3D11_USAGE_DYNAMIC,
				.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
				.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE
			};

			HRESULT hr = _backendResources.GetD3DDevice()->CreateBuffer(&bd, nullptr, _dynamicCB.put());
			if (FAILED(hr)) {
				Logger::Get().ComError("CreateBuffer 失败", hr);
				return nullptr;
			}

			break;
		}
	}

	return inOutTexture;
}

void Renderer::_UpdateActiveEffectDescs() noexcept {
	const uint32_t effectCount = (uint32_t)_effectDescs.size();
	const uint32_t drawerCount = (uint32_t)_effectDrawers.size();

	_activeEffectDescs.resize(drawerCount);

	for (uint32_t i = 0; i < effectCount; ++i) {
		_activeEffectDescs[i] = &_effectDescs[i];
	}

	if (drawerCount > effectCount) {
		// 已追加 Bicubic
		assert(drawerCount == effectCount + 1);
		_activeEffectDescs[effectCount] = &bicubicDesc;
	}
}

bool Renderer::_ShouldAppendBicubic(ID3D11Texture2D* outTexture) noexcept {
	const ScalingOptions& options = ScalingWindow::Get().Options();

	D3D11_TEXTURE2D_DESC texDesc;
	outTexture->GetDesc(&texDesc);
	const SIZE lastOutputSize = { (LONG)texDesc.Width, (LONG)texDesc.Height };
	const SIZE rendererSize = Win32Helper::GetSizeOfRect(ScalingWindow::Get().RendererRect());

	if (options.IsWindowedMode()) {
		// 窗口模式缩放时使用 Bicubic 放大。Bicubic (B=0, C=0.5) 的锐利度和 Lanczos 相差无几
		return lastOutputSize != rendererSize;
	} else {
		// 输出尺寸大于交换链尺寸则需要降采样
		return lastOutputSize.cx > rendererSize.cx || lastOutputSize.cy > rendererSize.cy;
	}
}

bool Renderer::_AppendBicubic(ID3D11Texture2D** inOutTexture) noexcept {
	const ScalingOptions& options = ScalingWindow::Get().Options();

	const EffectOption bicubicOption{
		.name = "Bicubic",
		.parameters{
			{"paramB", 0.0f},
			{"paramC", 0.5f}
		},
		.scalingType = options.IsWindowedMode() ? ScalingType::Fill : ScalingType::Fit
	};

	if (bicubicDesc.name.empty()) {
		// 参数不会改变，因此可以内联
		std::optional<EffectDesc> desc = CompileEffect(bicubicOption, true, true);
		if (!desc) {
			Logger::Get().Error("编译降采样效果失败");
			return false;
		}

		bicubicDesc = std::move(*desc);
	}

	EffectDrawer& bicubicDrawer = _effectDrawers.emplace_back();
	if (!bicubicDrawer.Initialize(
		bicubicDesc,
		bicubicOption,
		_backendResources,
		_backendDescriptorStore,
		inOutTexture
	)) {
		Logger::Get().Error("初始化降采样效果失败");
		return false;
	}

	return true;
}

ID3D11Texture2D* Renderer::_ResizeEffects() noexcept {
	const ScalingOptions& options = ScalingWindow::Get().Options();
	const std::vector<EffectOption>& effects = options.effects;
	assert(!effects.empty());
	const uint32_t effectCount = (uint32_t)effects.size();

	ID3D11Texture2D* inOutTexture = _frameSource->GetOutput();
	for (uint32_t i = 0; i < effectCount; ++i) {
		if (!_effectDrawers[i].ResizeTextures(
			_effectDescs[i],
			effects[i],
			_backendResources,
			&inOutTexture
		)) {
			Logger::Get().Error(fmt::format("更改效果#{} ({}) 尺寸失败", i, effects[i].name));
			return nullptr;
		}
	}

	// 处理追加的 Bicubic
	bool changed = false;
	if (_ShouldAppendBicubic(inOutTexture)) {
		if (_effectDrawers.size() > effectCount) {
			const EffectOption bicubicOption{
				.name = "Bicubic",
				.parameters{
					{"paramB", 0.0f},
					{"paramC", 0.5f}
				},
				.scalingType = options.IsWindowedMode() ? ScalingType::Fill : ScalingType::Fit
			};

			if (!_effectDrawers.back().ResizeTextures(
				bicubicDesc,
				bicubicOption,
				_backendResources,
				&inOutTexture
			)) {
				Logger::Get().Error("更改效果 Bicubic 尺寸失败");
				return nullptr;
			}
		} else {
			_AppendBicubic(&inOutTexture);
			changed = true;
		}
	} else {
		if (_effectDrawers.size() > effectCount) {
			_effectDrawers.resize(effectCount);
			changed = true;
		}
	}

	if (changed) {
		_UpdateActiveEffectDescs();
		_overlayDrawer.UpdateAfterActiveEffectsChanged();

		if (_effectsProfiler.IsProfiling()) {
			uint32_t passCount = 0;
			for (const EffectDesc* desc : _activeEffectDescs) {
				passCount += (uint32_t)desc->passes.size();
			}
			_effectsProfiler.SetPassCount(_backendResources.GetD3DDevice(), passCount);
		}
	}

	return inOutTexture;
}

void Renderer::_UpdateDestRect() noexcept {
	const RECT& rendererRect = ScalingWindow::Get().RendererRect();

	D3D11_TEXTURE2D_DESC desc;
	_frontendSharedTexture->GetDesc(&desc);

	_destRect.left = (rendererRect.left + rendererRect.right - (LONG)desc.Width) / 2;
	_destRect.top = (rendererRect.top + rendererRect.bottom - (LONG)desc.Height) / 2;
	_destRect.right = _destRect.left + (LONG)desc.Width;
	_destRect.bottom = _destRect.top + (LONG)desc.Height;
}

HANDLE Renderer::_CreateSharedTexture(ID3D11Texture2D* effectsOutput) noexcept {
	D3D11_TEXTURE2D_DESC desc;
	effectsOutput->GetDesc(&desc);
	SIZE textureSize = { (LONG)desc.Width, (LONG)desc.Height };

	// 创建共享纹理
	_backendSharedTexture = DirectXHelper::CreateTexture2D(
		_backendResources.GetD3DDevice(),
		DXGI_FORMAT_R8G8B8A8_UNORM,
		textureSize.cx,
		textureSize.cy,
		D3D11_BIND_SHADER_RESOURCE,
		D3D11_USAGE_DEFAULT,
		D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX
	);
	if (!_backendSharedTexture) {
		Logger::Get().Error("创建 Texture2D 失败");
		return NULL;
	}

	_backendSharedTextureMutex = _backendSharedTexture.try_as<IDXGIKeyedMutex>();

	winrt::com_ptr<IDXGIResource> sharedDxgiRes = _backendSharedTexture.try_as<IDXGIResource>();

	HANDLE sharedHandle = NULL;
	HRESULT hr = sharedDxgiRes->GetSharedHandle(&sharedHandle);
	if (FAILED(hr)) {
		Logger::Get().ComError("GetSharedHandle 失败", hr);
		return NULL;
	}

	return sharedHandle;
}

void Renderer::_BackendThreadProc() noexcept {
#ifdef _DEBUG
	SetThreadDescription(GetCurrentThread(), L"Magpie-缩放后端线程");
#endif

	winrt::init_apartment(winrt::apartment_type::single_threaded);

	if (const HANDLE sharedHandle = _InitBackend()) {
		_sharedTextureHandle.store(sharedHandle, std::memory_order_release);
		_sharedTextureHandle.notify_one();
	} else {
		_frameSource.reset();
		// 通知前端初始化失败
		_sharedTextureHandle.store(INVALID_HANDLE_VALUE, std::memory_order_release);
		_sharedTextureHandle.notify_one();

		// 即使失败也要创建消息循环，否则前端线程将一直等待
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0)) {
			DispatchMessage(&msg);
		}
		return;
	}

	StepTimerStatus stepTimerStatus = StepTimerStatus::WaitForNewFrame;
	const bool waitMsgForNewFrame =
		_frameSource->WaitType() == FrameSourceWaitType::WaitForMessage;

	MSG msg;
	while (true) {
		bool fpsUpdated = false;
		stepTimerStatus = _stepTimer.WaitForNextFrame(
			waitMsgForNewFrame && stepTimerStatus != StepTimerStatus::WaitForFPSLimiter,
			fpsUpdated
		);

		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				// 不能在前端线程释放
				_frameSource.reset();
				return;
			}

			DispatchMessage(&msg);
		}

		if (stepTimerStatus == StepTimerStatus::WaitForFPSLimiter) {
			// 新帧消息可能已被处理，之后的 WaitForNextFrame 不要等待消息，直到状态变化
			continue;
		}

		switch (_frameSource->Update()) {
		case FrameSourceState::Waiting:
			if (stepTimerStatus != StepTimerStatus::ForceNewFrame) {
				if (fpsUpdated) {
					// FPS 变化则要求前端重新渲染以更新叠加层，调整大小时这个操作十分必要
					PostMessage(ScalingWindow::Get().Handle(),
						CommonSharedConstants::WM_FRONTEND_RENDER, 0, 0);
				}
				break;
			}

			// 强制帧
			[[fallthrough]];
		case FrameSourceState::NewFrame:
			_BackendRender(_effectDrawers.back().GetOutputTexture());
			// 通知前端执行渲染
			PostMessage(ScalingWindow::Get().Handle(),
				CommonSharedConstants::WM_FRONTEND_RENDER, 0, 0);
			break;
		case FrameSourceState::Error:
			// 捕获出错，退出缩放
			ScalingWindow::Get().Dispatcher().TryEnqueue([]() {
				ScalingWindow::Get().RuntimeError(ScalingError::CaptureFailed);
				ScalingWindow::Get().Destroy();
			});

			while (GetMessage(&msg, NULL, 0, 0)) {
				DispatchMessage(&msg);
			}

			_frameSource.reset();
			return;
		}
	}
}

HANDLE Renderer::_InitBackend() noexcept {
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
			return NULL;
		}

		_backendThreadDispatcher = dqc.DispatcherQueue();
	}

	if (!_backendResources.Initialize(false)) {
		return NULL;
	}
	
	ID3D11Device5* d3dDevice = _backendResources.GetD3DDevice();
	_backendDescriptorStore.Initialize(d3dDevice);

	if (!_InitFrameSource()) {
		return NULL;
	}

	{
		std::optional<float> maxFrameRate;
		if (_frameSource->WaitType() == FrameSourceWaitType::NoWait) {
			// 某些捕获方式不会限制捕获帧率，因此将捕获帧率限制为屏幕刷新率
			const HWND hwndSrc = ScalingWindow::Get().SrcInfo().Handle();
			if (HMONITOR hMon = MonitorFromWindow(hwndSrc, MONITOR_DEFAULTTONEAREST)) {
				MONITORINFOEX mi{ sizeof(MONITORINFOEX) };
				GetMonitorInfo(hMon, &mi);

				DEVMODE dm{ .dmSize = sizeof(DEVMODE) };
				EnumDisplaySettings(mi.szDevice, ENUM_CURRENT_SETTINGS, &dm);

				if (dm.dmDisplayFrequency > 0) {
					Logger::Get().Info(fmt::format("屏幕刷新率: {}", dm.dmDisplayFrequency));
					maxFrameRate = float(dm.dmDisplayFrequency);
				}
			}
		}

		const ScalingOptions& options = ScalingWindow::Get().Options();
		if (options.maxFrameRate) {
			if (!maxFrameRate || *options.maxFrameRate < *maxFrameRate) {
				maxFrameRate = options.maxFrameRate;
			}
		}
		
		// 测试着色器性能时最小帧率应设为无限大，但由于 /fp:fast 下无限大不可靠，因此改为使用 max()，
		// 和无限大效果相同。
		const float minFrameRate = options.IsBenchmarkMode()
			? std::numeric_limits<float>::max() : options.minFrameRate;
		_stepTimer.Initialize(minFrameRate, maxFrameRate);
	}

	ID3D11Texture2D* outputTexture = _BuildEffects();
	if (!outputTexture) {
		return NULL;
	}

	HRESULT hr = d3dDevice->CreateFence(
		_fenceValue, D3D11_FENCE_FLAG_NONE, IID_PPV_ARGS(&_d3dFence));
	if (FAILED(hr)) {
		// GH#979
		// 这个错误会在某些很旧的显卡上出现，似乎是驱动的 bug。文档中提到 ID3D11Device5::CreateFence 
		// 和 ID3D12Device::CreateFence 等价，但支持 DX12 的显卡也有失败的可能，如 GH#1013
		Logger::Get().ComError("CreateFence 失败", hr);
		_backendInitError = ScalingError::CreateFenceFailed;
		return NULL;
	}

	if (!_fenceEvent.try_create(wil::EventOptions::None, nullptr)) {
		Logger::Get().Win32Error("CreateEvent 失败");
		return NULL;
	}

	HANDLE sharedHandle = _CreateSharedTexture(outputTexture);
	if (!sharedHandle) {
		Logger::Get().Error("_CreateSharedTexture 失败");
		return NULL;
	}

	// 最后启动捕获以尽可能推迟显示黄色边框 (Win10) 或禁用圆角 (Win11)
	if (!_frameSource->Start()) {
		Logger::Get().Error("启动捕获失败");
		return NULL;
	}

	return sharedHandle;
}

void Renderer::_BackendRender(ID3D11Texture2D* effectsOutput) noexcept {
	_stepTimer.PrepareForRender();

	ID3D11DeviceContext4* d3dDC = _backendResources.GetD3DDC();
	d3dDC->ClearState();

	if (ID3D11Buffer* t = _dynamicCB.get()) {
		_UpdateDynamicConstants();
		d3dDC->CSSetConstantBuffers(1, 1, &t);
	}

	_effectsProfiler.OnBeginEffects(d3dDC);

	for (const EffectDrawer& effectDrawer : _effectDrawers) {
		effectDrawer.Draw(_effectsProfiler);
	}

	_effectsProfiler.OnEndEffects(d3dDC);

	HRESULT hr = d3dDC->Signal(_d3dFence.get(), ++_fenceValue);
	if (FAILED(hr)) {
		Logger::Get().ComError("Signal 失败", hr);
		return;
	}

	hr = _d3dFence->SetEventOnCompletion(_fenceValue, _fenceEvent.get());
	if (FAILED(hr)) {
		Logger::Get().ComError("SetEventOnCompletion 失败", hr);
		return;
	}

	d3dDC->Flush();

	// 等待渲染完成
	_fenceEvent.wait();

	// 查询效果的渲染时间
	_effectsProfiler.QueryTimings(d3dDC);

	// 渲染完成后再更新 _sharedTextureMutexKey，否则前端必须等待，降低光标流畅度
	const uint64_t key = ++_sharedTextureMutexKey;
	hr = _backendSharedTextureMutex->AcquireSync(key - 1, INFINITE);
	if (FAILED(hr)) {
		Logger::Get().ComError("AcquireSync 失败", hr);
		return;
	}

	d3dDC->CopyResource(_backendSharedTexture.get(), effectsOutput);

	_backendSharedTextureMutex->ReleaseSync(key);

	// 根据 https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11device-opensharedresource，
	// 更新共享纹理后必须调用 Flush
	d3dDC->Flush();
}

bool Renderer::_UpdateDynamicConstants() const noexcept {
	// cbuffer __CB2 : register(b1) { uint __frameCount; };

	ID3D11DeviceContext4* d3dDC = _backendResources.GetD3DDC();

	D3D11_MAPPED_SUBRESOURCE ms;
	HRESULT hr = d3dDC->Map(_dynamicCB.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
	if (SUCCEEDED(hr)) {
		// 避免使用 *(uint32_t*)ms.pData，见
		// https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-map
		const uint32_t frameCount = _stepTimer.FrameCount();
		std::memcpy(ms.pData, &frameCount, 4);
		d3dDC->Unmap(_dynamicCB.get(), 0);
	} else {
		Logger::Get().ComError("Map 失败", hr);
		return false;
	}

	return true;
}

winrt::IAsyncAction Renderer::_UpdateNextScreenshotNum(const wchar_t* imgFormat) noexcept {
	// 由于中途会转到后台，应防止并发计算 _screenshotNum
	static wil::srwlock screenshotNumLock;

	wil::rwlock_release_exclusive_scope_exit lk;
	while (true) {
		lk = screenshotNumLock.try_lock_exclusive();
		if (lk) {
			break;
		} else {
			// 前一次截图正在执行 FindUnusedScreenshotNum，给它继续执行的机会直到释放锁
			co_await _backendThreadDispatcher;
		}
	}

	const std::filesystem::path& screenshotsDir = ScalingWindow::Get().Options().screenshotsDir;

	if (_screenshotNum != 0) {
		if (_screenshotNum == std::numeric_limits<uint32_t>::max()) {
			// 如果达到 UINT_MAX 应重新寻找可用序号，除了特意构造的数据不可能出现这种情况
			_screenshotNum = 0;
		} else {
			++_screenshotNum;

			if (Win32Helper::DirExists(screenshotsDir.c_str())) {
				const std::wstring fileName =
					fmt::format(L"{}\\Magpie_{:03}.{}", screenshotsDir.native(), _screenshotNum, imgFormat);
				if (Win32Helper::FileExists(fileName.c_str())) {
					// 下一个序号不可用则需要重新寻找可用序号
					_screenshotNum = 0;
				}
			}
		}
	}

	if (_screenshotNum == 0) {
		co_await winrt::resume_background();
		// 如果已有截图很多可能较耗时，转到后台防止阻塞后端线程
		const uint32_t screenshotNum = ScreenshotHelper::FindUnusedScreenshotNum(screenshotsDir);
		co_await _backendThreadDispatcher;

		// FindUnusedScreenshotNum 失败则始终使用 001
		_screenshotNum = screenshotNum == 0 ? 1 : screenshotNum;
	}
}

winrt::IAsyncOperation<bool> Renderer::_TakeScreenshotImpl(
	uint32_t effectIdx,
	uint32_t passIdx,
	uint32_t outputIdx
) noexcept {
	co_await _backendThreadDispatcher;

	// 最后一个通道的输出即 OUTPUT 不会被覆盖，可以直接使用。
	// 倒数第二个通道的输出也不会被覆盖，因为最后一个通道只会写入 OUTPUT。
	// 从倒数第三个通道开始需要检查输出是否被后面的通道覆盖。
	bool isOverwritten = false;
	ID3D11Texture2D* sourceTex;
	EffectIntermediateTextureFormat format;
	// 效果输出保存为 png，中间结果保存为 dds
	const wchar_t* imgFormat;

	if (passIdx == std::numeric_limits<uint32_t>::max()) {
		sourceTex = _effectDrawers[effectIdx].GetOutputTexture();
		format = _activeEffectDescs[effectIdx]->textures[1].format;
		imgFormat = L"png";
	} else {
		const std::vector<EffectPassDesc>& passes = _activeEffectDescs[effectIdx]->passes;
		const uint32_t passCount = (uint32_t)passes.size();

		const SmallVector<uint32_t>& outputs = passes[passIdx].outputs;
		// 只有一个输出时才允许不提供 outputIdx
		assert(outputIdx != std::numeric_limits<uint32_t>::max() || outputs.size() == 1);
		const uint32_t targetOutput =
			outputIdx == std::numeric_limits<uint32_t>::max() ? outputs[0] : outputs[outputIdx];

		sourceTex = _effectDrawers[effectIdx].GetTexture(targetOutput);
		format = _activeEffectDescs[effectIdx]->textures[targetOutput].format;
		imgFormat = targetOutput == 1 ? L"png" : L"dds";

		if (passIdx + 3 <= passCount) {
			// 检查 targetOutput 是否被后面的通道修改 
			for (uint32_t i = passIdx + 1, end = passCount - 1; i < end; ++i) {
				const SmallVector<uint32_t>& curOutputs = passes[i].outputs;
				if (std::find(curOutputs.begin(), curOutputs.end(), targetOutput) != curOutputs.end()) {
					isOverwritten = true;
					break;
				}
			}
		}
	}

	co_await _UpdateNextScreenshotNum(imgFormat);
	// 读取纹理数据时 _screenshotNum 有被并发修改的可能，把当前值保存到本地
	const uint32_t screenshotNum = _screenshotNum;

	ID3D11Device5* d3dDevice = _backendResources.GetD3DDevice();
	ID3D11DeviceContext4* d3dDC = _backendResources.GetD3DDC();

	if (isOverwritten) {
		// 重新渲染
		d3dDC->ClearState();

		if (ID3D11Buffer* t = _dynamicCB.get()) {
			d3dDC->CSSetConstantBuffers(1, 1, &t);
		}

		_effectDrawers[effectIdx].DrawForExport(*_activeEffectDescs[effectIdx], passIdx);
	}

	// 创建 staging 纹理
	D3D11_TEXTURE2D_DESC desc;
	sourceTex->GetDesc(&desc);
	desc.Usage = D3D11_USAGE_STAGING;
	desc.BindFlags = 0;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.MiscFlags = 0;

	winrt::com_ptr<ID3D11Texture2D> stagingTex;
	HRESULT hr = d3dDevice->CreateTexture2D(
		&desc, nullptr, stagingTex.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateTexture2D 失败", hr);
		co_return false;
	}

	d3dDC->CopyResource(stagingTex.get(), sourceTex);
	
	// 如果要导出的纹理不会被覆盖则转到后台等待 GPU 以防止卡顿
	if (!isOverwritten) {
		// 为避免混乱，使用独立的栅栏
		winrt::com_ptr<ID3D11Fence> localFence;
		wil::unique_event_nothrow localFenceEvent;

		hr = d3dDevice->CreateFence(
			0, D3D11_FENCE_FLAG_NONE, IID_PPV_ARGS(&localFence));
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateFence 失败", hr);
			co_return false;
		}

		if (!localFenceEvent.try_create(wil::EventOptions::None, nullptr)) {
			Logger::Get().Win32Error("CreateEvent 失败");
			co_return false;
		}

		hr = d3dDC->Signal(localFence.get(), 1);
		if (FAILED(hr)) {
			Logger::Get().ComError("Signal 失败", hr);
			co_return false;
		}

		hr = localFence->SetEventOnCompletion(1, localFenceEvent.get());
		if (FAILED(hr)) {
			Logger::Get().ComError("SetEventOnCompletion 失败", hr);
			co_return false;
		}

		d3dDC->Flush();

		winrt::DispatcherQueue dispatcher = _backendThreadDispatcher;
		co_await winrt::resume_background();
		localFenceEvent.wait();
		co_await dispatcher;
	}

	// 读取纹理数据到内存。isOverwritten 为真时这个调用将阻塞 CPU
	D3D11_MAPPED_SUBRESOURCE mapped;
	hr = d3dDC->Map(stagingTex.get(), 0, D3D11_MAP_READ, 0, &mapped);
	if (FAILED(hr)) {
		Logger::Get().ComError("Map 失败", hr);
		co_return false;
	}

	std::vector<uint8_t> pixelData(size_t(mapped.RowPitch) * desc.Height);
	std::memcpy(pixelData.data(), mapped.pData, pixelData.size());

	d3dDC->Unmap(stagingTex.get(), 0);

	co_await winrt::resume_background();

	// 确保截图保存目录存在
	const std::filesystem::path& screenshotsDir = ScalingWindow::Get().Options().screenshotsDir;
	if (!Win32Helper::CreateDir(screenshotsDir.c_str(), true)) {
		Logger::Get().Error("CreateDir 失败");
		co_return false;
	}

	std::wstring fileName = fmt::format(L"Magpie_{:03}.{}", screenshotNum, imgFormat);
	const std::filesystem::path& fullPath = screenshotsDir / fileName;

	if (!TextureHelper::SaveTexture(
		fullPath.c_str(), desc.Width, desc.Height, format, pixelData, mapped.RowPitch)) {
		Logger::Get().Error("SaveImage 失败");
		co_return false;
	}

	winrt::hstring successMsg =
		ScalingWindow::Get().GetLocalizedString(L"Message_ScreenshotSaved");
	ScalingWindow::Get().ShowToast(
		fmt::format(fmt::runtime(std::wstring_view(successMsg)), fileName));
	co_return true;
}

// 监听 PrintScreen 实现截屏时隐藏光标
LRESULT CALLBACK Renderer::_LowLevelKeyboardHook(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode != HC_ACTION || wParam != WM_KEYDOWN) {
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	}

	KBDLLHOOKSTRUCT* info = (KBDLLHOOKSTRUCT*)lParam;
	if (info->vkCode == VK_SNAPSHOT) {
		// 为了缩短钩子处理时间，异步执行所有逻辑
		ScalingWindow::Get().Dispatcher().TryEnqueue([]() -> winrt::fire_and_forget {
			// 暂时隐藏光标
			Renderer& renderer = ScalingWindow::Get().Renderer();
			renderer._cursorDrawer.IsCursorVisible(false);
			renderer._FrontendRender();

			const HWND hwndScaling = ScalingWindow::Get().Handle();

			winrt::DispatcherQueue dispatcher = ScalingWindow::Get().Dispatcher();
			co_await 200ms;
			co_await dispatcher;

			if (ScalingWindow::Get().Handle() == hwndScaling &&
				!renderer._cursorDrawer.IsCursorVisible()
			) {
				renderer._cursorDrawer.IsCursorVisible(true);
				renderer._FrontendRender();
			}
		});
	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

}
