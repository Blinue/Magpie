#include "pch.h"
#include "Renderer.h"
#include "MagApp.h"
#include "Win32Utils.h"
#include "StrUtils.h"
#include "EffectCompiler.h"
#include "FrameSourceBase.h"
#include "DeviceResources.h"
#include "GPUTimer.h"
#include "EffectDrawer.h"
#include "OverlayDrawer.h"
#include "Logger.h"
#include "CursorManager.h"
#include "WindowHelper.h"
#include "Utils.h"

namespace Magpie::Core {

Renderer::Renderer() {}

Renderer::~Renderer() {}

bool Renderer::Initialize() {
	_gpuTimer.reset(new GPUTimer());

	if (!GetWindowRect(MagApp::Get().GetHwndSrc(), &_srcWndRect)) {
		Logger::Get().Win32Error("GetWindowRect 失败");
		return false;
	}

	if (!_BuildEffects()) {
		Logger::Get().Error("_BuildEffects 失败");
		return false;
	}

	if (MagApp::Get().GetOptions().IsShowFPS()) {
		_overlayDrawer.reset(new OverlayDrawer());
		if (!_overlayDrawer->Initialize()) {
			_overlayDrawer.reset();
			Logger::Get().Error("初始化 OverlayDrawer 失败");
		}
	}

	// 初始化所有效果共用的动态常量缓冲区
	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bd.ByteWidth = 4 * (UINT)_dynamicConstants.size();
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	HRESULT hr = MagApp::Get().GetDeviceResources().GetD3DDevice()
		->CreateBuffer(&bd, nullptr, _dynamicCB.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateBuffer 失败", hr);
		return false;
	}

	return true;
}


void Renderer::Render(bool onPrint) {
	int srcState = _CheckSrcState();
	if (srcState != 0) {
		Logger::Get().Info("源窗口状态改变，退出全屏");
		MagApp::Get().Stop(srcState == 2);
		return;
	}

	DeviceResources& dr = MagApp::Get().GetDeviceResources();

	if (!_waitingForNextFrame) {
		dr.BeginFrame();
		_gpuTimer->OnBeginFrame();
	}

	// 首先处理配置改变产生的回调
	// MagApp::Get().GetOptions().OnBeginFrame();

	auto state = onPrint ? FrameSourceBase::UpdateState::NoUpdate : MagApp::Get().GetFrameSource().Update();
	_waitingForNextFrame = state == FrameSourceBase::UpdateState::Waiting
		|| state == FrameSourceBase::UpdateState::Error;
	if (_waitingForNextFrame) {
		return;
	}

	MagApp::Get().GetCursorManager().OnBeginFrame();

	if (!_UpdateDynamicConstants()) {
		Logger::Get().Error("_UpdateDynamicConstants 失败");
	}

	auto d3dDC = dr.GetD3DDC();

	{
		ID3D11Buffer* t = _dynamicCB.get();
		d3dDC->CSSetConstantBuffers(0, 1, &t);
	}

	{
		SIZE outputSize = Win32Utils::GetSizeOfRect(_outputRect);
		SIZE hostSize = Win32Utils::GetSizeOfRect(MagApp::Get().GetHostWndRect());
		if (outputSize.cx < hostSize.cx || outputSize.cy < hostSize.cy) {
			// 存在黑边时渲染每帧前清空后缓冲区
			ID3D11UnorderedAccessView* backBufferUAV = nullptr;
			dr.GetUnorderedAccessView(dr.GetBackBuffer(), &backBufferUAV);
			static const UINT black[4] = { 0,0,0,255 };
			d3dDC->ClearUnorderedAccessViewUint(backBufferUAV, black);
		}
	}

	_gpuTimer->OnBeginEffects();

	uint32_t idx = 0;
	if (state == FrameSourceBase::UpdateState::NoUpdate) {
		// 此帧内容无变化
		// 从第一个使用动态常量的效果开始渲染
		// 如果没有则只渲染最后一个效果的最后一个通道

		size_t i = 0;
		for (size_t end = _effects.size() - 1; i < end; ++i) {
			if (_effects[i].IsUseDynamic()) {
				break;
			} else {
				for (uint32_t j = (uint32_t)_effects[i].GetDesc().passes.size(); j > 0; --j) {
					_gpuTimer->OnEndPass(idx++);
				}
			}
		}

		if (i == _effects.size()) {
			// 只渲染最后一个 Effect 的最后一个 pass
			_effects.back().Draw(idx, true);
		} else {
			for (; i < _effects.size(); ++i) {
				_effects[i].Draw(idx);
			}
		}
	} else {
		for (auto& effect : _effects) {
			effect.Draw(idx);
		}
	}

	_gpuTimer->OnEndEffects();

	if (_overlayDrawer) {
		_overlayDrawer->Draw();
	}

	dr.EndFrame();
}

bool Renderer::IsUIVisiable() const noexcept {
	return _overlayDrawer ? _overlayDrawer->IsUIVisiable() : false;
}

void Renderer::SetUIVisibility(bool value) {
	if (!value) {
		if (_overlayDrawer && _overlayDrawer->IsUIVisiable()) {
			_overlayDrawer->SetUIVisibility(false);
			_gpuTimer->StopProfiling();
		}
		return;
	}

	if (!_overlayDrawer) {
		_overlayDrawer.reset(new OverlayDrawer());
		if (!_overlayDrawer->Initialize()) {
			_overlayDrawer.reset();
			Logger::Get().Error("初始化 OverlayDrawer 失败");
			return;
		}
	}

	if (!_overlayDrawer->IsUIVisiable()) {
		_overlayDrawer->SetUIVisibility(true);

		uint32_t passCount = 0;
		for (const auto& effect : _effects) {
			passCount += (uint32_t)effect.GetDesc().passes.size();
		}

		// StartProfiling 必须在 OnBeginFrame 之前调用
		_gpuTimer->StartProfiling(500ms, passCount);
	}
}

bool CheckForeground(HWND hwndForeground) {
	std::wstring className = Win32Utils::GetWndClassName(hwndForeground);

	if (!WindowHelper::IsValidSrcWindow(hwndForeground)) {
		return true;
	}

	RECT rectForground{};

	// 如果捕获模式可以捕获到弹窗，则允许小的弹窗
	if (MagApp::Get().GetFrameSource().IsScreenCapture()
		&& GetWindowStyle(hwndForeground) & (WS_POPUP | WS_CHILD)
	) {
		if (!Win32Utils::GetWindowFrameRect(hwndForeground, rectForground)) {
			Logger::Get().Error("GetWindowFrameRect 失败");
			return false;
		}

		// 弹窗如果完全在源窗口客户区内则不退出全屏
		const RECT& srcFrameRect = MagApp::Get().GetFrameSource().GetSrcFrameRect();
		if (rectForground.left >= srcFrameRect.left
			&& rectForground.right <= srcFrameRect.right
			&& rectForground.top >= srcFrameRect.top
			&& rectForground.bottom <= srcFrameRect.bottom
			) {
			return true;
		}
	}

	if (rectForground == RECT{}) {
		if (!Win32Utils::GetWindowFrameRect(hwndForeground, rectForground)) {
			Logger::Get().Error("GetWindowFrameRect 失败");
			return false;
		}
	}

	IntersectRect(&rectForground, &MagApp::Get().GetHostWndRect(), &rectForground);

	// 允许稍微重叠，否则前台窗口最大化时会意外退出
	return rectForground.right - rectForground.left < 10 || rectForground.right - rectForground.top < 10;
}

uint32_t Renderer::GetEffectCount() const noexcept {
	return (uint32_t)_effects.size();
}

const EffectDesc& Renderer::GetEffectDesc(uint32_t idx) const noexcept {
	assert(idx < _effects.size());
	return _effects[idx].GetDesc();
}

// 0 -> 可继续缩放
// 1 -> 前台窗口改变或源窗口最大化（如果不允许缩放最大化的窗口）/最小化
// 2 -> 源窗口大小或位置改变或最大化（如果允许缩放最大化的窗口）
int Renderer::_CheckSrcState() {
	HWND hwndSrc = MagApp::Get().GetHwndSrc();
	const MagOptions& options = MagApp::Get().GetOptions();

	if (!options.IsDebugMode()) {
		HWND hwndForeground = GetForegroundWindow();
		// 在 3D 游戏模式下打开游戏内叠加层则全屏窗口可以接收焦点
		if (!options.Is3DGameMode() || !IsUIVisiable() || hwndForeground != MagApp::Get().GetHwndHost()) {
			if (hwndForeground && hwndForeground != hwndSrc && !CheckForeground(hwndForeground)) {
				Logger::Get().Info("前台窗口已改变");
				return 1;
			}
		}
	}

	UINT showCmd = Win32Utils::GetWindowShowCmd(hwndSrc);
	if (showCmd != SW_NORMAL && (showCmd != SW_SHOWMAXIMIZED || !options.IsAllowScalingMaximized())) {
		Logger::Get().Info("源窗口显示状态改变");
		return 1;
	}

	RECT rect;
	if (!GetWindowRect(hwndSrc, &rect)) {
		Logger::Get().Error("GetWindowRect 失败");
		return 1;
	}

	if (_srcWndRect != rect) {
		Logger::Get().Info("源窗口位置或大小改变");
		return 2;
	}

	return 0;
}

static bool CompileEffect(bool isLastEffect, const EffectOption& option, EffectDesc& result) {
	result.name = StrUtils::UTF16ToUTF8(option.name);
	// 将文件夹分隔符统一为 '\'
	for (char& c : result.name) {
		if (c == '/') {
			c = '\\';
		}
	}

	result.flags = isLastEffect ? EffectFlags::LastEffect : 0;

	if (option.flags & EffectOptionFlags::InlineParams) {
		result.flags |= EffectFlags::InlineParams;
	}
	if (option.flags & EffectOptionFlags::FP16) {
		result.flags |= EffectFlags::FP16;
	}

	uint32_t compileFlag = 0;
	MagOptions& options = MagApp::Get().GetOptions();
	if (options.IsDisableEffectCache()) {
		compileFlag |= EffectCompilerFlags::NoCache;
	}
	if (options.IsSaveEffectSources()) {
		compileFlag |= EffectCompilerFlags::SaveSources;
	}
	if (options.IsWarningsAreErrors()) {
		compileFlag |= EffectCompilerFlags::WarningsAreErrors;
	}

	bool success = true;
	int duration = Utils::Measure([&]() {
		success = !EffectCompiler::Compile(result, compileFlag, &option.parameters);
	});

	if (success) {
		Logger::Get().Info(fmt::format("编译 {}.hlsl 用时 {} 毫秒", StrUtils::UTF16ToUTF8(option.name), duration / 1000.0f));
	} else {
		Logger::Get().Error(StrUtils::Concat("编译 ", StrUtils::UTF16ToUTF8(option.name), ".hlsl 失败"));
	}
	return success;
}

bool Renderer::_BuildEffects() {
	const std::vector<EffectOption>& effectsOption = MagApp::Get().GetOptions().effects;
	uint32_t effectCount = (int)effectsOption.size();
	if (effectCount == 0) {
		return false;
	}

	// 并行编译所有效果
	std::vector<EffectDesc> effectDescs(effectsOption.size());
	std::atomic<bool> anyFailure;

	int duration = Utils::Measure([&]() {
		Win32Utils::RunParallel([&](uint32_t id) {
			if (!CompileEffect(id == effectCount - 1, effectsOption[id], effectDescs[id])) {
				anyFailure.store(true, std::memory_order_relaxed);
			}
		}, effectCount);
	});

	if (anyFailure.load(std::memory_order_relaxed)) {
		return false;
	}

	if (effectCount > 1) {
		Logger::Get().Info(fmt::format("编译着色器总计用时 {} 毫秒", duration / 1000.0f));
	}

	ID3D11Texture2D* effectInput = MagApp::Get().GetFrameSource().GetOutput();

	DownscalingEffect& downscalingEffect = MagApp::Get().GetOptions().downscalingEffect;
	if (!downscalingEffect.name.empty()) {
		_effects.reserve(effectsOption.size() + 1);
	}
	_effects.resize(effectsOption.size());

	for (uint32_t i = 0; i < effectCount; ++i) {
		bool isLastEffect = i == effectCount - 1;

		if (!_effects[i].Initialize(
			effectDescs[i], effectsOption[i], effectInput,
			isLastEffect ? &_outputRect : nullptr,
			isLastEffect ? &_virtualOutputRect : nullptr
		)) {
			Logger::Get().Error(fmt::format("初始化效果#{} ({}) 失败", i, StrUtils::UTF16ToUTF8(effectsOption[i].name)));
			return false;
		}

		effectInput = _effects[i].GetOutputTexture();
	}
	
	if (!downscalingEffect.name.empty()) {
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
						anyFailure.store(true, std::memory_order_relaxed);
					}
				}, 2);
			});

			if (anyFailure.load(std::memory_order_relaxed)) {
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
	}

	return true;
}

bool Renderer::_UpdateDynamicConstants() {
	// cbuffer __CB1 : register(b0) {
	//     int4 __cursorRect;
	//     float2 __cursorPt;
	//     uint2 __cursorPos;
	//     uint __cursorType;
	//     uint __frameCount;
	// };

	CursorManager& cursorManager = MagApp::Get().GetCursorManager();
	if (cursorManager.HasCursor() && !(MagApp::Get().GetOptions().Is3DGameMode() && IsUIVisiable())) {
		const POINT* pos = cursorManager.GetCursorPos();
		const CursorManager::CursorInfo* ci = cursorManager.GetCursorInfo();

		ID3D11Texture2D* cursorTex;
		CursorManager::CursorType cursorType = CursorManager::CursorType::Color;
		if (!cursorManager.GetCursorTexture(&cursorTex, cursorType)) {
			Logger::Get().Error("GetCursorTexture 失败");
		}
		assert(pos && ci);

		float cursorScaling = (float)MagApp::Get().GetOptions().cursorScaling;
		if (cursorScaling < 1e-5) {
			SIZE srcFrameSize = Win32Utils::GetSizeOfRect(MagApp::Get().GetFrameSource().GetSrcFrameRect());
			SIZE virtualOutputSize = Win32Utils::GetSizeOfRect(_virtualOutputRect);
			cursorScaling = (((float)virtualOutputSize.cx / srcFrameSize.cx)
				+ ((float)virtualOutputSize.cy / srcFrameSize.cy)) / 2;
		}

		SIZE cursorSize = {
			std::lroundf(ci->size.cx * cursorScaling),
			std::lroundf(ci->size.cy * cursorScaling)
		};

		_dynamicConstants[0].intVal = pos->x - std::lroundf(ci->hotSpot.x * cursorScaling);
		_dynamicConstants[1].intVal = pos->y - std::lroundf(ci->hotSpot.y * cursorScaling);
		_dynamicConstants[2].intVal = _dynamicConstants[0].intVal + cursorSize.cx;
		_dynamicConstants[3].intVal = _dynamicConstants[1].intVal + cursorSize.cy;

		_dynamicConstants[4].floatVal = 1.0f / cursorSize.cx;
		_dynamicConstants[5].floatVal = 1.0f / cursorSize.cy;

		_dynamicConstants[6].uintVal = pos->x;
		_dynamicConstants[7].uintVal = pos->y;

		_dynamicConstants[8].uintVal = (uint32_t)cursorType;
	} else {
		_dynamicConstants[0].intVal = INT_MAX;
		_dynamicConstants[1].intVal = INT_MAX;
		_dynamicConstants[2].intVal = INT_MAX;
		_dynamicConstants[3].intVal = INT_MAX;
		_dynamicConstants[6].uintVal = UINT_MAX;
		_dynamicConstants[7].uintVal = UINT_MAX;
	}

	_dynamicConstants[9].uintVal = _gpuTimer->GetFrameCount();

	auto d3dDC = MagApp::Get().GetDeviceResources().GetD3DDC();

	D3D11_MAPPED_SUBRESOURCE ms;
	HRESULT hr = d3dDC->Map(_dynamicCB.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
	if (SUCCEEDED(hr)) {
		std::memcpy(ms.pData, _dynamicConstants.data(), _dynamicConstants.size() * 4);
		d3dDC->Unmap(_dynamicCB.get(), 0);
	} else {
		Logger::Get().ComError("Map 失败", hr);
		return false;
	}

	return true;
}

}
