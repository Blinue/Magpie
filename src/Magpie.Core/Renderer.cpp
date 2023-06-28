#include "pch.h"
#include "Renderer.h"
#include "DeviceResources.h"
#include "ScalingOptions.h"
#include "Logger.h"
#include "Win32Utils.h"
#include "EffectDrawer.h"
#include "StrUtils.h"
#include "Utils.h"
#include "EffectCompiler.h"
#include "GraphicsCaptureFrameSource.h"
#include "GDIFrameSource.h"
#include "DirectXHelper.h"
#include <dispatcherqueue.h>

namespace Magpie::Core {

Renderer::Renderer() noexcept {}

Renderer::~Renderer() noexcept {
	if (_backendThread.joinable()) {
		DWORD backendThreadId = GetThreadId(_backendThread.native_handle());
		// 持续尝试直到 _backendThread 创建了消息队列
		while (!PostThreadMessage(backendThreadId, WM_QUIT, 0, 0)) {
			Sleep(1);
		}
		_backendThread.join();
	}
}

bool Renderer::Initialize(HWND hwndSrc, HWND hwndScaling, const ScalingOptions& options) noexcept {
	_hwndSrc = hwndSrc;
	_hwndScaling = hwndScaling;

	GetWindowRect(_hwndScaling, &_scalingWndRect);

	_backendThread = std::thread(std::bind(&Renderer::_BackendThreadProc, this, options));

	if (!_frontendResources.Initialize(options)) {
		Logger::Get().Error("初始化前端资源失败");
		return false;
	}

	if (!_CreateSwapChain()) {
		Logger::Get().Error("_CreateSwapChain 失败");
		return false;
	}

	// 等待后端初始化完成
	while (true) {
		{
			std::scoped_lock lk(_mutex);
			if (_sharedTextureHandle) {
				if (_sharedTextureHandle == INVALID_HANDLE_VALUE) {
					Logger::Get().Error("后端初始化失败");
					return false;
				}
				break;
			}
		}
		// 将时间片让给后端线程
		Sleep(0);
	}

	// 获取共享纹理
	HRESULT hr = _frontendResources.GetD3DDevice()->OpenSharedResource(
		_sharedTextureHandle, IID_PPV_ARGS(_frontendSharedTexture.put()));
	if (FAILED(hr)) {
		Logger::Get().ComError("OpenSharedResource 失败", hr);
		return false;
	}

	_frontendSharedTextureMutex = _frontendSharedTexture.try_as<IDXGIKeyedMutex>();

	D3D11_TEXTURE2D_DESC desc;
	_frontendSharedTexture->GetDesc(&desc);

	_destRect.left = (_scalingWndRect.left + _scalingWndRect.right - desc.Width) / 2;
	_destRect.top = (_scalingWndRect.top + _scalingWndRect.bottom - desc.Height) / 2;
	_destRect.right = _destRect.left + desc.Width;
	_destRect.bottom = _destRect.top + desc.Height;

	RECT viewportRect{
		_destRect.left - _scalingWndRect.left,
		_destRect.top - _scalingWndRect.top,
		_destRect.right - _scalingWndRect.left,
		_destRect.bottom - _scalingWndRect.top
	};
	if (!_cursorDrawer.Initialize(_frontendResources, _backBuffer.get(), viewportRect, options)) {
		Logger::Get().ComError("初始化 CursorDrawer 失败", hr);
		return false;
	}

	return true;
}

static bool CheckMultiplaneOverlaySupport(IDXGISwapChain4* swapChain) noexcept {
	winrt::com_ptr<IDXGIOutput> output;
	HRESULT hr = swapChain->GetContainingOutput(output.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("获取 IDXGIOutput 失败", hr);
		return false;
	}

	winrt::com_ptr<IDXGIOutput2> output2 = output.try_as<IDXGIOutput2>();
	if (!output2) {
		Logger::Get().Info("获取 IDXGIOutput2 失败");
		return false;
	}

	return output2->SupportsOverlays();
}

void Renderer::OnCursorVisibilityChanged(bool isVisible) {
	_backendThreadDqc.DispatcherQueue().TryEnqueue([this, isVisible]() {
		_frameSource->OnCursorVisibilityChanged(isVisible);
	});
}

bool Renderer::_CreateSwapChain() noexcept {
	DXGI_SWAP_CHAIN_DESC1 sd{};
	sd.Width = _scalingWndRect.right - _scalingWndRect.left;
	sd.Height = _scalingWndRect.bottom - _scalingWndRect.top;
	sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	sd.SampleDesc.Count = 1;
	sd.Scaling = DXGI_SCALING_NONE;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	// 为了降低延迟，两个垂直同步之间允许渲染 3 帧
	sd.BufferCount = 4;
	// 渲染每帧之前都会清空后缓冲区，因此无需 DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	// 只要显卡支持始终启用 DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING 以支持可变刷新率
	sd.Flags = (_frontendResources.IsSupportTearing() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0)
		| DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

	winrt::com_ptr<IDXGISwapChain1> dxgiSwapChain = nullptr;
	HRESULT hr = _frontendResources.GetDXGIFactory()->CreateSwapChainForHwnd(
		_frontendResources.GetD3DDevice(),
		_hwndScaling,
		&sd,
		nullptr,
		nullptr,
		dxgiSwapChain.put()
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("创建交换链失败", hr);
		return false;
	}

	_swapChain = dxgiSwapChain.try_as<IDXGISwapChain4>();
	if (!_swapChain) {
		Logger::Get().Error("获取 IDXGISwapChain2 失败");
		return false;
	}

	// 允许提前渲染 3 帧
	_swapChain->SetMaximumFrameLatency(3);

	_frameLatencyWaitableObject.reset(_swapChain->GetFrameLatencyWaitableObject());
	if (!_frameLatencyWaitableObject) {
		Logger::Get().Error("GetFrameLatencyWaitableObject 失败");
		return false;
	}

	hr = _frontendResources.GetDXGIFactory()->MakeWindowAssociation(_hwndScaling, DXGI_MWA_NO_ALT_ENTER);
	if (FAILED(hr)) {
		Logger::Get().ComError("MakeWindowAssociation 失败", hr);
	}

	hr = _swapChain->GetBuffer(0, IID_PPV_ARGS(_backBuffer.put()));
	if (FAILED(hr)) {
		Logger::Get().ComError("获取后缓冲区失败", hr);
		return false;
	}

	// 检查 Multiplane Overlay 支持
	const bool supportMPO = CheckMultiplaneOverlaySupport(_swapChain.get());
	Logger::Get().Info(StrUtils::Concat("Multiplane Overlay 支持：", supportMPO ? "是" : "否"));

	return true;
}

void Renderer::Render(HCURSOR hCursor, POINT cursorPos) noexcept {
	// 有新帧或光标改变则渲染新的帧
	if (_lastAccessMutexKey == _sharedTextureMutexKey) {
		if (_lastAccessMutexKey == 0 || (hCursor == _lastCursorHandle && cursorPos == _lastCursorPos)) {
			// 第一帧尚未完成或光标没有移动
			return;
		}
	}

	_lastCursorHandle = hCursor;
	_lastCursorPos = cursorPos;

	WaitForSingleObjectEx(_frameLatencyWaitableObject.get(), 1000, TRUE);

	ID3D11DeviceContext4* d3dDC = _frontendResources.GetD3DDC();
	d3dDC->ClearState();

	// 所有渲染都使用三角形带拓扑
	d3dDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	ID3D11RenderTargetView* backBufferRtv = _frontendResources.GetRenderTargetView(_backBuffer.get());

	// 输出画面是否充满缩放窗口
	const bool isFill = _destRect == _scalingWndRect;

	if (!isFill) {
		// 以黑色填充背景，因为我们指定了 DXGI_SWAP_EFFECT_FLIP_DISCARD，同时也是为了和 RTSS 兼容
		static constexpr FLOAT BLACK[4] = { 0.0f,0.0f,0.0f,1.0f };
		d3dDC->ClearRenderTargetView(backBufferRtv, BLACK);
	}

	_lastAccessMutexKey = ++_sharedTextureMutexKey;
	HRESULT hr = _frontendSharedTextureMutex->AcquireSync(_lastAccessMutexKey - 1, INFINITE);
	if (FAILED(hr)) {
		return;
	}

	if (isFill) {
		d3dDC->CopyResource(_backBuffer.get(), _frontendSharedTexture.get());
	} else {
		d3dDC->CopySubresourceRegion(
			_backBuffer.get(),
			0,
			_destRect.left - _scalingWndRect.left,
			_destRect.top-_scalingWndRect.top,
			0,
			_frontendSharedTexture.get(),
			0,
			nullptr
		);
	}

	_frontendSharedTextureMutex->ReleaseSync(_lastAccessMutexKey);

	// 绘制光标
	_cursorDrawer.Draw(hCursor, cursorPos);

	// 两个垂直同步之间允许渲染数帧，SyncInterval = 0 只呈现最新的一帧，旧帧被丢弃
	_swapChain->Present(0, 0);

	// 丢弃渲染目标的内容。仅当现有内容将被完全覆盖时，此操作才有效
	d3dDC->DiscardView(backBufferRtv);
}

bool Renderer::_InitFrameSource(const ScalingOptions& options) noexcept {
	switch (options.captureMethod) {
	case CaptureMethod::GraphicsCapture:
		_frameSource = std::make_unique<GraphicsCaptureFrameSource>();
		break;
	/*case CaptureMethod::DesktopDuplication:
		frameSource = std::make_unique<DesktopDuplicationFrameSource>();
		break;*/
	case CaptureMethod::GDI:
		_frameSource = std::make_unique<GDIFrameSource>();
		break;
	/*case CaptureMethod::DwmSharedSurface:
		frameSource = std::make_unique<DwmSharedSurfaceFrameSource>();
		break;*/
	default:
		Logger::Get().Error("未知的捕获模式");
		return false;
	}

	Logger::Get().Info(StrUtils::Concat("当前捕获模式：", _frameSource->Name()));

	if (!_frameSource->Initialize(_hwndSrc, _hwndScaling, options, _backendResources)) {
		Logger::Get().Error("初始化 FrameSource 失败");
		return false;
	}

	D3D11_TEXTURE2D_DESC desc;
	_frameSource->GetOutput()->GetDesc(&desc);
	Logger::Get().Info(fmt::format("源窗口尺寸：{}x{}", desc.Width, desc.Height));

	return true;
}

static std::optional<EffectDesc> CompileEffect(
	const ScalingOptions& scalingOptions,
	const EffectOption& effectOption
) noexcept {
	EffectDesc result;

	result.name = StrUtils::UTF16ToUTF8(effectOption.name);

	if (effectOption.flags & EffectOptionFlags::InlineParams) {
		result.flags |= EffectFlags::InlineParams;
	}
	if (effectOption.flags & EffectOptionFlags::FP16) {
		result.flags |= EffectFlags::FP16;
	}

	uint32_t compileFlag = 0;
	if (scalingOptions.IsDisableEffectCache()) {
		compileFlag |= EffectCompilerFlags::NoCache;
	}
	if (scalingOptions.IsSaveEffectSources()) {
		compileFlag |= EffectCompilerFlags::SaveSources;
	}
	if (scalingOptions.IsWarningsAreErrors()) {
		compileFlag |= EffectCompilerFlags::WarningsAreErrors;
	}

	bool success = true;
	int duration = Utils::Measure([&]() {
		success = !EffectCompiler::Compile(result, compileFlag, &effectOption.parameters);
	});

	if (success) {
		Logger::Get().Info(fmt::format("编译 {}.hlsl 用时 {} 毫秒",
			StrUtils::UTF16ToUTF8(effectOption.name), duration / 1000.0f));
		return result;
	} else {
		Logger::Get().Error(StrUtils::Concat("编译 ",
			StrUtils::UTF16ToUTF8(effectOption.name), ".hlsl 失败"));
		return std::nullopt;
	}
}

ID3D11Texture2D* Renderer::_BuildEffects(const ScalingOptions& options) noexcept {
	assert(!options.effects.empty());

	const uint32_t effectCount = (uint32_t)options.effects.size();

	// 并行编译所有效果
	std::vector<EffectDesc> effectDescs(options.effects.size());
	std::atomic<bool> allSuccess = true;

	int duration = Utils::Measure([&]() {
		Win32Utils::RunParallel([&](uint32_t id) {
			std::optional<EffectDesc> desc = CompileEffect(options, options.effects[id]);
			if (desc) {
				effectDescs[id] = std::move(*desc);
			} else {
				allSuccess = false;
			}
		}, effectCount);
	});

	if (!allSuccess) {
		return nullptr;
	}

	if (effectCount > 1) {
		Logger::Get().Info(fmt::format("编译着色器总计用时 {} 毫秒", duration / 1000.0f));
	}

	const SIZE scalingWndSize = Win32Utils::GetSizeOfRect(_scalingWndRect);

	_effectDrawers.resize(options.effects.size());

	ID3D11Texture2D* inOutTexture = _frameSource->GetOutput();
	for (uint32_t i = 0; i < effectCount; ++i) {
		if (!_effectDrawers[i].Initialize(
			effectDescs[i],
			options.effects[i],
			_backendResources,
			scalingWndSize,
			&inOutTexture
		)) {
			Logger::Get().Error(fmt::format("初始化效果#{} ({}) 失败", i, StrUtils::UTF16ToUTF8(options.effects[i].name)));
			return nullptr;
		}
	}

	// 输出尺寸大于缩放窗口尺寸则需要降采样
	D3D11_TEXTURE2D_DESC desc;
	inOutTexture->GetDesc(&desc);
	if ((LONG)desc.Width > scalingWndSize.cx || (LONG)desc.Height > scalingWndSize.cy) {
		EffectOption bicubicOption;
		bicubicOption.name = L"Bicubic";
		bicubicOption.parameters[L"paramB"] = 0.0f;
		bicubicOption.parameters[L"paramC"] = 0.5f;
		bicubicOption.scalingType = ScalingType::Fit;
		// 参数不会改变，因此可以内联
		bicubicOption.flags = EffectOptionFlags::InlineParams;

		std::optional<EffectDesc> bicubicDesc = CompileEffect(options, bicubicOption);
		if (!bicubicDesc) {
			Logger::Get().Error("编译降采样效果失败");
			return nullptr;
		}

		EffectDrawer& bicubicDrawer = _effectDrawers.emplace_back();
		if (!bicubicDrawer.Initialize(
			*bicubicDesc,
			bicubicOption,
			_backendResources,
			scalingWndSize,
			&inOutTexture
		)) {
			Logger::Get().Error("初始化降采样效果失败");
			return nullptr;
		}
	}

	// 初始化所有效果共用的动态常量缓冲区
	{
		for (uint32_t i = 0; i < effectDescs.size(); ++i) {
			if(effectDescs[i].flags & EffectFlags::UseDynamic) {
				_firstDynamicEffectIdx = i;
				break;
			}
		}
		
		if (_firstDynamicEffectIdx != std::numeric_limits<uint32_t>::max()) {
			D3D11_BUFFER_DESC bd{};
			bd.Usage = D3D11_USAGE_DYNAMIC;
			bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			bd.ByteWidth = 16;	// 只用 4 个字节
			bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

			HRESULT hr = _backendResources.GetD3DDevice()->CreateBuffer(&bd, nullptr, _dynamicCB.put());
			if (FAILED(hr)) {
				Logger::Get().ComError("CreateBuffer 失败", hr);
				return nullptr;
			}
		}
	}

	return inOutTexture;
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
		Logger::Get().Error("GetSharedHandle 失败");
		return NULL;
	}

	return sharedHandle;
}

void Renderer::_BackendThreadProc(const ScalingOptions& options) noexcept {
	winrt::init_apartment(winrt::apartment_type::single_threaded);

	ID3D11Texture2D* outputTexture = _InitBackend(options);
	if (!outputTexture) {
		_frameSource.reset();
		// 通知前端初始化失败
		{
			std::scoped_lock lk(_mutex);
			_sharedTextureHandle = INVALID_HANDLE_VALUE;
		}

		// 即使失败也要创建消息循环，否则前端线程将一直等待
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0)) {
			DispatchMessage(&msg);
		}
		return;
	}

	enum {
		WaitForStepTimer,
		WaitForFrameSource,
		DuplicateFrame
	} renderState = WaitForStepTimer;

	MSG msg;
	while (true) {
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				// 不能在前端线程释放
				_frameSource.reset();
				return;
			}

			DispatchMessage(&msg);
		}

		if (renderState != WaitForFrameSource) {
			// 实际上向 StepTimer 汇报重复帧有一帧的滞后，不过无伤大雅
			if (!_stepTimer.NewFrame(renderState == DuplicateFrame)) {
				continue;
			}
			renderState = WaitForFrameSource;
		}

		switch (_frameSource->Update()) {
		case FrameSourceBase::UpdateState::NewFrame:
			_BackendRender(outputTexture, false);
			renderState = WaitForStepTimer;
			break;
		case FrameSourceBase::UpdateState::NoChange:
			// 源窗口内容不变，也没有动态效果则跳过渲染
			if (_dynamicCB) {
				_BackendRender(outputTexture, true);
				renderState = WaitForStepTimer;
			} else {
				renderState = DuplicateFrame;
			}
			break;
		case FrameSourceBase::UpdateState::Waiting:
			// 等待新消息
			WaitMessage();
			break;
		default:
			renderState = WaitForStepTimer;
			break;
		}
	}
}

ID3D11Texture2D* Renderer::_InitBackend(const ScalingOptions& options) noexcept {
	// 创建 DispatcherQueue
	{
		DispatcherQueueOptions dqOptions{};
		dqOptions.dwSize = sizeof(DispatcherQueueOptions);
		dqOptions.threadType = DQTYPE_THREAD_CURRENT;

		HRESULT hr = CreateDispatcherQueueController(
			dqOptions,
			(PDISPATCHERQUEUECONTROLLER*)winrt::put_abi(_backendThreadDqc)
		);
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateDispatcherQueueController 失败", hr);
			return nullptr;
		}
	}

	{
		std::optional<float> frameRateLimit;
		// 渲染帧率最大为屏幕刷新率，这是某些捕获方法的要求，也可以提高 Graphics Capture 的流畅度
		if (HMONITOR hMon = MonitorFromWindow(_hwndSrc, MONITOR_DEFAULTTONEAREST)) {
			MONITORINFOEX mi{ sizeof(MONITORINFOEX) };
			GetMonitorInfo(hMon, &mi);

			DEVMODE dm{};
			dm.dmSize = sizeof(DEVMODE);
			EnumDisplaySettings(mi.szDevice, ENUM_CURRENT_SETTINGS, &dm);

			if (dm.dmDisplayFrequency > 0) {
				Logger::Get().Info(fmt::format("屏幕刷新率：{}", dm.dmDisplayFrequency));
				frameRateLimit = (float)dm.dmDisplayFrequency;
			}
		}

		if (options.maxFrameRate) {
			if (!frameRateLimit || *options.maxFrameRate < *frameRateLimit) {
				frameRateLimit = options.maxFrameRate;
			}
		}

		_stepTimer.Initialize(frameRateLimit);
	}

	if (!_backendResources.Initialize(options)) {
		return nullptr;
	}

	if (!_InitFrameSource(options)) {
		return nullptr;
	}

	ID3D11Texture2D* outputTexture = _BuildEffects(options);
	if (!outputTexture) {
		return nullptr;
	}

	HRESULT hr = _backendResources.GetD3DDevice()->CreateFence(
		_fenceValue, D3D11_FENCE_FLAG_NONE, IID_PPV_ARGS(&_d3dFence));
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateFence 失败", hr);
		return nullptr;
	}

	_fenceEvent.reset(Win32Utils::SafeHandle(CreateEvent(nullptr, FALSE, FALSE, nullptr)));
	if (!_fenceEvent) {
		Logger::Get().Win32Error("CreateEvent 失败");
		return nullptr;
	}

	HANDLE sharedHandle = _CreateSharedTexture(outputTexture);
	if (!sharedHandle) {
		Logger::Get().Win32Error("_CreateSharedTexture 失败");
		return nullptr;
	}

	{
		std::scoped_lock lk(_mutex);
		_sharedTextureHandle = sharedHandle;
		_srcRect = _frameSource->SrcRect();
	}

	return outputTexture;
}

void Renderer::_BackendRender(ID3D11Texture2D* effectsOutput, bool noChange) noexcept {
	ID3D11DeviceContext4* d3dDC = _backendResources.GetD3DDC();
	d3dDC->ClearState();

	if (ID3D11Buffer* t = _dynamicCB.get()) {
		_UpdateDynamicConstants();
		d3dDC->CSSetConstantBuffers(1, 1, &t);
	}

	if (noChange) {
		// 源窗口内容不变则从第一个动态效果开始渲染
		for (uint32_t i = _firstDynamicEffectIdx; i < _effectDrawers.size(); ++i) {
			_effectDrawers[i].Draw();
		}
	} else {
		for (const EffectDrawer& effectDrawer : _effectDrawers) {
			effectDrawer.Draw();
		}
	}

	HRESULT hr = d3dDC->Signal(_d3dFence.get(), ++_fenceValue);
	if (FAILED(hr)) {
		return;
	}
	hr = _d3dFence->SetEventOnCompletion(_fenceValue, _fenceEvent.get());
	if (FAILED(hr)) {
		return;
	}

	d3dDC->Flush();

	// 等待渲染完成
	WaitForSingleObject(_fenceEvent.get(), INFINITE);

	// 渲染完成后再更新 _sharedTextureMutexKey，否则前端必须等待，会大幅降低帧率
	const uint64_t key = ++_sharedTextureMutexKey;
	hr = _backendSharedTextureMutex->AcquireSync(key - 1, INFINITE);
	if (FAILED(hr)) {
		return;
	}

	d3dDC->CopyResource(_backendSharedTexture.get(), effectsOutput);

	_backendSharedTextureMutex->ReleaseSync(key);

	// 根据 https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11device-opensharedresource，
	// 更新共享纹理后必须调用 Flush
	d3dDC->Flush();

	// 唤醒前台线程
	PostMessage(_hwndScaling, WM_NULL, 0, 0);
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
