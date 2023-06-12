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

	_backendThread = std::thread(std::bind(&Renderer::_BackendThreadProc, this, options));

	if (!_frontendResources.Initialize(hwndScaling, options)) {
		return false;
	}

	return true;
}

void Renderer::Render() noexcept {
	
}

bool Renderer::_InitFrameSource(const ScalingOptions& options) noexcept {
	switch (options.captureMethod) {
	case CaptureMethod::GraphicsCapture:
		_frameSource = std::make_unique<GraphicsCaptureFrameSource>();
		break;
	/*case CaptureMethod::DesktopDuplication:
		frameSource = std::make_unique<DesktopDuplicationFrameSource>();
		break;
	case CaptureMethod::GDI:
		frameSource = std::make_unique<GDIFrameSource>();
		break;
	case CaptureMethod::DwmSharedSurface:
		frameSource = std::make_unique<DwmSharedSurfaceFrameSource>();
		break;*/
	default:
		Logger::Get().Error("未知的捕获模式");
		return false;
	}

	Logger::Get().Info(StrUtils::Concat("当前捕获模式：", _frameSource->GetName()));

	ID3D11Device5* d3dDevice = _backendDeviceResources.GetD3DDevice();
	if (!_frameSource->Initialize(_hwndSrc, _hwndScaling, options, d3dDevice)) {
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

bool Renderer::_BuildEffects(const ScalingOptions& options) noexcept {
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
		return false;
	}

	if (effectCount > 1) {
		Logger::Get().Info(fmt::format("编译着色器总计用时 {} 毫秒", duration / 1000.0f));
	}

	/*DownscalingEffect& downscalingEffect = MagApp::Get().GetOptions().downscalingEffect;
	if (!downscalingEffect.name.empty()) {
		_effects.reserve(effectsOption.size() + 1);
	}*/
	_effectDrawers.resize(options.effects.size());

	RECT scalingWndRect;
	GetWindowRect(_hwndScaling, &scalingWndRect);
	SIZE scalingWndSize = Win32Utils::GetSizeOfRect(scalingWndRect);

	ID3D11Texture2D* inOutTexture = _frameSource->GetOutput();
	for (uint32_t i = 0; i < effectCount; ++i) {
		if (!_effectDrawers[i].Initialize(
			effectDescs[i],
			options.effects[i],
			_backendDeviceResources,
			scalingWndSize,
			&inOutTexture
		)) {
			Logger::Get().Error(fmt::format("初始化效果#{} ({}) 失败", i, StrUtils::UTF16ToUTF8(options.effects[i].name)));
			return false;
		}
	}

	/*if (!downscalingEffect.name.empty()) {
		const SIZE hostSize = Win32Utils::GetSizeOfRect(MagApp::Get().GetHostWndRect());
		const SIZE outputSize = Win32Utils::GetSizeOfRect(_virtualOutputRect);
		if (outputSize.cx > hostSize.cx || outputSize.cy > hostSize.cy) {
			// 需降采样
			EffectOption downscalingEffectOption;
			downscalingEffectOption.name = downscalingEffect.name;
			downscalingEffectOption.parameters = downscalingEffect.parameters;
			downscalingEffectOption.scalingType = ScalingType::Fit;
			downscalingEffectOption.flags = EffectOptionFlags::InlineParams;	// 内联参数

			EffectDesc downscalingEffectDesc;

			// 最后一个效果需重新编译
			// 在分离光标渲染逻辑后这里可优化
			duration = Utils::Measure([&]() {
				Win32Utils::RunParallel([&](uint32_t id) {
					if (!CompileEffect(
						id == 1,
						id == 0 ? effectsOption.back() : downscalingEffectOption,
						id == 0 ? effectDescs.back() : downscalingEffectDesc
						)) {
						allSuccess = false;
					}
				}, 2);
			});

			if (!allSuccess) {
				return false;
			}

			Logger::Get().Info(fmt::format("编译降采样着色器用时 {} 毫秒", duration / 1000.0f));

			_effects.pop_back();
			if (_effects.empty()) {
				effectInput = MagApp::Get().GetFrameSource().GetOutput();
			} else {
				effectInput = _effects.back().GetOutputTexture();
			}

			_effects.resize(_effects.size() + 2);

			// 重新构建最后一个效果
			const size_t originLastEffectIdx = _effects.size() - 2;
			if (!_effects[originLastEffectIdx].Initialize(effectDescs.back(), effectsOption.back(),
				effectInput, nullptr, nullptr)
				) {
				Logger::Get().Error(fmt::format("初始化效果#{} ({}) 失败",
					originLastEffectIdx, StrUtils::UTF16ToUTF8(effectsOption.back().name)));
				return false;
			}
			effectInput = _effects[originLastEffectIdx].GetOutputTexture();

			// 构建降采样效果
			if (!_effects.back().Initialize(downscalingEffectDesc, downscalingEffectOption,
				effectInput, &_outputRect, &_virtualOutputRect
				)) {
				Logger::Get().Error(fmt::format("初始化降采样效果 ({}) 失败",
					StrUtils::UTF16ToUTF8(downscalingEffect.name)));
			}
		}
	}*/

	return true;
}

void Renderer::_BackendThreadProc(const ScalingOptions& options) noexcept {
	winrt::init_apartment(winrt::apartment_type::single_threaded);

	if (!_backendDeviceResources.Initialize(_hwndScaling, options)) {
		return;
	}

	ID3D11Device5* d3dDevice = _backendDeviceResources.GetD3DDevice();

	if (!_InitFrameSource(options)) {
		return;
	}

	if (!_BuildEffects(options)) {
		return;
	}
	
	HRESULT hr = d3dDevice->CreateFence(_fenceValue, D3D11_FENCE_FLAG_NONE, IID_PPV_ARGS(&_d3dFence));
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateFence 失败", hr);
		return;
	}

	_fenceEvent.reset(Win32Utils::SafeHandle(CreateEvent(nullptr, FALSE, FALSE, nullptr)));
	if (!_fenceEvent) {
		Logger::Get().Win32Error("CreateEvent 失败");
		return;
	}

	MSG msg;
	while (true) {
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				// 不能在前台线程释放
				_frameSource.reset();
				return;
			}

			DispatchMessage(&msg);
		}
		
		_BackendRender();

		// 等待新消息 1ms
		MsgWaitForMultipleObjectsEx(0, nullptr, 1, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
	}
}

void Renderer::_BackendRender() noexcept {
	FrameSourceBase::UpdateState updateState = _frameSource->Update();
	if (updateState != FrameSourceBase::UpdateState::NewFrame) {
		return;
	}

	winrt::com_ptr<ID3D11DeviceContext4> d3dDC;
	{
		winrt::com_ptr<ID3D11DeviceContext> t;
		_backendDeviceResources.GetD3DDevice()->GetImmediateContext(t.put());
		d3dDC = t.try_as<ID3D11DeviceContext4>();
	}

	for (const EffectDrawer& effectDrawer : _effectDrawers) {
		effectDrawer.Draw(d3dDC.get());
	}

	// 等待渲染完成
	HRESULT hr = d3dDC->Signal(_d3dFence.get(), ++_fenceValue);
	if (FAILED(hr)) {
		return;
	}
	hr = _d3dFence->SetEventOnCompletion(_fenceValue, _fenceEvent.get());
	if (FAILED(hr)) {
		return;
	}
	
	WaitForSingleObject(_fenceEvent.get(), INFINITE);
}

}
