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
#include <dispatcherqueue.h>
#include "ScalingWindow.h"
#include "OverlayDrawer.h"
#include "CursorManager.h"
#include "EffectsProfiler.h"
#include "CommonSharedConstants.h"
#include "SwapChainPresenter.h"
#include "DCompPresenter.h"

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

static void LogAdapter(IDXGIAdapter4* adapter) noexcept {
	DXGI_ADAPTER_DESC1 desc;
	adapter->GetDesc1(&desc);

	Logger::Get().Info(fmt::format("当前图形适配器: \n\tVendorId: {:#x}\n\tDeviceId: {:#x}\n\tDescription: {}",
		desc.VendorId, desc.DeviceId, StrHelper::UTF16ToUTF8(desc.Description)));
}

ScalingError Renderer::Initialize(HWND hwndAttach) noexcept {
	_backendThread = std::thread(std::bind(&Renderer::_BackendThreadProc, this));

	if (!_frontendResources.Initialize()) {
		Logger::Get().Error("初始化前端资源失败");
		return ScalingError::ScalingFailedGeneral;
	}

	LogAdapter(_frontendResources.GetGraphicsAdapter());

	if (!_InitPresenter(hwndAttach)) {
		Logger::Get().Error("_InitPresenter 失败");
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

	if (!_cursorDrawer.Initialize(_frontendResources)) {
		Logger::Get().ComError("初始化 CursorDrawer 失败", hr);
		return ScalingError::ScalingFailedGeneral;
	}

	if (ScalingWindow::Get().Options().IsShowFPS()) {
		_overlayDrawer.reset(new OverlayDrawer());
		if (!_overlayDrawer->Initialize(&_frontendResources)) {
			Logger::Get().Error("初始化 OverlayDrawer 失败");
			return ScalingError::ScalingFailedGeneral;
		}
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
	if (_overlayDrawer) {
		_overlayDrawer->MessageHandler(msg, wParam, lParam);

		// 有些鼠标操作需要渲染 ImGui 多次，见 https://github.com/ocornut/imgui/issues/2268
		if (msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MOUSEWHEEL ||
			msg == WM_MOUSEHWHEEL || msg == WM_LBUTTONUP || msg == WM_RBUTTONUP) {
			_FrontendRender();
		}
	}
}

bool Renderer::_InitPresenter(HWND hwndAttach) noexcept {
	const ScalingOptions& options = ScalingWindow::Get().Options();
	// DirectComposition 呈现的特点：
	// 1. 调整大小时闪烁更少，因此适合窗口化缩放。
	// 2. 不使用交换链，因此不会触发 DirectFlip。
	if (options.IsWindowedMode() || options.IsDirectFlipDisabled()) {
		_presenter = std::make_unique<DCompPresenter>();
	} else {
		_presenter = std::make_unique<SwapChainPresenter>();
	}
	
	return _presenter->Initialize(hwndAttach, _frontendResources);
}

void Renderer::_FrontendRender() noexcept {
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

	// 绘制叠加层
	if (_overlayDrawer) {
		// ImGui 至少渲染两遍，否则经常有布局错误
		_overlayDrawer->Draw(
			2,
			_stepTimer.FPS(),
			_overlayDrawer->IsUIVisible() ? _effectsProfiler.GetTimings() : SmallVector<float>(),
			drawOffset
		);
	}

	// 绘制光标
	_cursorDrawer.Draw(frameTex.get(), drawOffset);
	
	_presenter->EndFrame();
}

bool Renderer::Render() noexcept {
	const CursorManager& cursorManager = ScalingWindow::Get().CursorManager();
	const HCURSOR hCursor = cursorManager.Cursor();
	const POINT cursorPos = cursorManager.CursorPos();
	const uint32_t fps = _stepTimer.FPS();

	// 有新帧或光标改变则渲染新的帧
	if (_lastAccessMutexKey == _sharedTextureMutexKey.load(std::memory_order_relaxed)) {
		if (_lastAccessMutexKey == 0) {
			// 第一帧尚未完成
			return false;
		}

		// 检查光标是否移动
		if (hCursor == _lastCursorHandle && cursorPos == _lastCursorPos) {
			if (IsOverlayVisible() || ScalingWindow::Get().Options().IsShowFPS()) {
				// 检查 FPS 是否变化
				if (fps == _lastFPS) {
					return false;
				}
			} else {
				return false;
			}
		}
	}

	_lastCursorHandle = hCursor;
	_lastCursorPos = cursorPos;
	_lastFPS = fps;

	_FrontendRender();
	return true;
}

bool Renderer::Resize() noexcept {
	if (!_presenter->Resize()) {
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

		// 渲染完成再通知前端防止黑屏。前端会自动执行渲染，因此无需发送 WM_FOREGROUND_RENDER
		_BackendRender(outputTexture);

		_sharedTextureHandle.store(sharedHandle, std::memory_order_release);
		_sharedTextureHandle.notify_one();
	});

	// 等待后端更改分辨率和渲染
	_sharedTextureHandle.wait(NULL, std::memory_order_relaxed);
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

void Renderer::MoveSwapChain() noexcept {
	_UpdateDestRect();
}

bool Renderer::IsOverlayVisible() noexcept {
	return _overlayDrawer && _overlayDrawer->IsUIVisible();
}

void Renderer::SetOverlayVisibility(bool value) noexcept {
	if (value) {
		if (!_overlayDrawer) {
			_overlayDrawer = std::make_unique<OverlayDrawer>();
			if (!_overlayDrawer->Initialize(&_frontendResources)) {
				_overlayDrawer.reset();
				Logger::Get().Error("初始化 OverlayDrawer 失败");
				return;
			}
		}

		if (_overlayDrawer->IsUIVisible()) {
			return;
		}
		_overlayDrawer->SetUIVisibility(true);

		_backendThreadDispatcher.TryEnqueue([this]() {
			uint32_t passCount = 0;
			for (const EffectInfo& info : _effectInfos) {
				passCount += (uint32_t)info.passNames.size();
			}
			_effectsProfiler.Start(_backendResources.GetD3DDevice(), passCount);
		});
	} else {
		if (_overlayDrawer) {
			if (!_overlayDrawer->IsUIVisible()) {
				return;
			}
			_overlayDrawer->SetUIVisibility(false);
		}

		_backendThreadDispatcher.TryEnqueue([this]() {
			_effectsProfiler.Stop();
		});
	}

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

// 单位为微秒
template<typename Fn>
static int Measure(const Fn& func) noexcept {
	using namespace std::chrono;

	auto t = steady_clock::now();
	func();
	auto dura = duration_cast<microseconds>(steady_clock::now() - t);

	return int(dura.count());
}

static std::optional<EffectDesc> CompileEffect(
	const EffectOption& effectOption,
	bool noFP16,
	bool forceInlineParams = false
) noexcept {
	// 指定效果名
	EffectDesc result{
		.name = StrHelper::UTF16ToUTF8(effectOption.name)
	};

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
	int duration = Measure([&]() {
		success = !EffectCompiler::Compile(result, compileFlag, &effectOption.parameters);
	});

	if (success) {
		Logger::Get().Info(fmt::format("编译 {}.hlsl 用时 {} 毫秒",
			StrHelper::UTF16ToUTF8(effectOption.name), duration / 1000.0f));
		return result;
	} else {
		Logger::Get().Error(StrHelper::Concat("编译 ",
			StrHelper::UTF16ToUTF8(effectOption.name), ".hlsl 失败"));
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
		// 窗口模式缩放时最后一个效果如果支持缩放则将 Fit 视为 Fill
		if (!_effectDrawers[i].Initialize(
			_effectDescs[i],
			effects[i],
			options.IsWindowedMode() && i == effectCount - 1,
			_backendResources,
			_backendDescriptorStore,
			&inOutTexture
		)) {
			Logger::Get().Error(fmt::format("初始化效果#{} ({}) 失败", i, StrHelper::UTF16ToUTF8(effects[i].name)));
			return nullptr;
		}

		// 释放 CSO 内存，不再需要它们
		for (EffectPassDesc& passDesc : _effectDescs[i].passes) {
			passDesc.cso = nullptr;
		}
	}

	// 初始化 _effectInfos
	_effectInfos.resize(_effectDescs.size());
	for (size_t i = 0; i < _effectDescs.size(); ++i) {
		EffectInfo& info = _effectInfos[i];
		EffectDesc& desc = _effectDescs[i];
		info.name = std::move(desc.name);

		info.passNames.reserve(desc.passes.size());
		for (EffectPassDesc& passDesc : desc.passes) {
			info.passNames.emplace_back(std::move(passDesc.desc));
		}

		info.isFP16 = desc.passes[0].flags & EffectPassFlags::UseFP16;
	}
	
	if (_ShouldAppendBicubic(inOutTexture)) {
		if (!_AppendBicubicIfNecessary(&inOutTexture)) {
			Logger::Get().Error("_AppendBicubicIfNecessary 失败");
			return nullptr;
		}
	}

	// 初始化所有效果共用的动态常量缓冲区
	for (const EffectDesc& effectDesc : _effectDescs) {
		for (const EffectPassDesc& passDesc : effectDesc.passes) {
			if (passDesc.flags & EffectPassFlags::UseDynamic) {
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

		if (_dynamicCB) {
			break;
		}
	}

	return inOutTexture;
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

bool Renderer::_AppendBicubicIfNecessary(ID3D11Texture2D** inOutTexture) noexcept {
	const ScalingOptions& options = ScalingWindow::Get().Options();

	const EffectOption bicubicOption{
		.name = L"Bicubic",
		.parameters{
			{L"paramB", 0.0f},
			{L"paramC", 0.5f}
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
		false,
		_backendResources,
		_backendDescriptorStore,
		inOutTexture
	)) {
		Logger::Get().Error("初始化降采样效果失败");
		return false;
	}

	// 为降采样算法生成 EffectInfo
	EffectInfo& bicubicEffectInfo = _effectInfos.emplace_back();
	bicubicEffectInfo.name = bicubicDesc.name;
	bicubicEffectInfo.passNames.reserve(bicubicDesc.passes.size());
	for (const EffectPassDesc& passDesc : bicubicDesc.passes) {
		bicubicEffectInfo.passNames.push_back(passDesc.desc);
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
		// 窗口模式缩放时最后一个效果如果支持缩放则将 Fit 视为 Fill
		if (!_effectDrawers[i].ResizeTextures(
			_effectDescs[i],
			effects[i],
			options.IsWindowedMode() && i == effectCount - 1,
			_backendResources,
			&inOutTexture
		)) {
			Logger::Get().Error(fmt::format("更改效果#{} ({}) 尺寸失败", i, StrHelper::UTF16ToUTF8(effects[i].name)));
			return nullptr;
		}
	}

	// 处理追加的 Bicubic
	if (_ShouldAppendBicubic(inOutTexture)) {
		if (_effectDrawers.size() > effectCount) {
			const EffectOption bicubicOption{
				.name = L"Bicubic",
				.parameters{
					{L"paramB", 0.0f},
					{L"paramC", 0.5f}
				},
				.scalingType = options.IsWindowedMode() ? ScalingType::Fill : ScalingType::Fit
			};

			if (!_effectDrawers.back().ResizeTextures(
				bicubicDesc,
				bicubicOption,
				false,
				_backendResources,
				&inOutTexture
			)) {
				Logger::Get().Error("更改效果 Bicubic 尺寸失败");
				return nullptr;
			}
		} else {
			_AppendBicubicIfNecessary(&inOutTexture);
		}
	} else {
		if (_effectDrawers.size() > effectCount) {
			_effectDrawers.resize(effectCount);
			_effectInfos.resize(effectCount);
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
	SetThreadDescription(GetCurrentThread(), L"Magpie 缩放后端线程");
#endif

	winrt::init_apartment(winrt::apartment_type::single_threaded);

	if (ID3D11Texture2D* outputTexture = _InitBackend(); !outputTexture) {
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
						CommonSharedConstants::WM_FOREGROUND_RENDER, 0, 0);
				}
				break;
			}

			// 强制帧
			[[fallthrough]];
		case FrameSourceState::NewFrame:
			_BackendRender(_effectDrawers.back().GetOutputTexture());
			// 通知前端执行渲染
			PostMessage(ScalingWindow::Get().Handle(),
				CommonSharedConstants::WM_FOREGROUND_RENDER, 0, 0);
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

ID3D11Texture2D* Renderer::_InitBackend() noexcept {
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
			return nullptr;
		}

		_backendThreadDispatcher = dqc.DispatcherQueue();
	}

	if (!_backendResources.Initialize()) {
		return nullptr;
	}
	
	ID3D11Device5* d3dDevice = _backendResources.GetD3DDevice();
	_backendDescriptorStore.Initialize(d3dDevice);

	if (!_InitFrameSource()) {
		return nullptr;
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
		return nullptr;
	}

	HRESULT hr = d3dDevice->CreateFence(
		_fenceValue, D3D11_FENCE_FLAG_NONE, IID_PPV_ARGS(&_d3dFence));
	if (FAILED(hr)) {
		// GH#979
		// 这个错误会在某些很旧的显卡上出现，似乎是驱动的 bug。文档中提到 ID3D11Device5::CreateFence 
		// 和 ID3D12Device::CreateFence 等价，但支持 DX12 的显卡也有失败的可能，如 GH#1013
		Logger::Get().ComError("CreateFence 失败", hr);
		_backendInitError = ScalingError::CreateFenceFailed;
		return nullptr;
	}

	if (!_fenceEvent.try_create(wil::EventOptions::None, nullptr)) {
		Logger::Get().Win32Error("CreateEvent 失败");
		return nullptr;
	}

	HANDLE sharedHandle = _CreateSharedTexture(outputTexture);
	if (!sharedHandle) {
		Logger::Get().Win32Error("_CreateSharedTexture 失败");
		return nullptr;
	}
	
	_sharedTextureHandle.store(sharedHandle, std::memory_order_release);
	_sharedTextureHandle.notify_one();

	return outputTexture;
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

}
