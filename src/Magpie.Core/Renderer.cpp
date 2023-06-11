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

bool Renderer::Initialize(HWND /*hwndSrc*/, HWND hwndScaling, const ScalingOptions& options) noexcept {
	_backendThread = std::thread(std::bind(&Renderer::_BackendThreadProc, this, hwndScaling, options));

	_frontendResources = std::make_unique<DeviceResources>();
	if (!_frontendResources->Initialize(hwndScaling, options)) {
		return false;
	}

	return true;
}

void Renderer::Render() noexcept {
	
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

static std::vector<EffectDrawer> BuildEffects(
	const ScalingOptions& scalingOptions,
	RECT& outputRect,
	RECT& virtualOutputRect
) noexcept {
	assert(!scalingOptions.effects.empty());

	std::vector<EffectDrawer> effectDrawers;

	const uint32_t effectCount = (uint32_t)scalingOptions.effects.size();

	// 并行编译所有效果
	std::vector<EffectDesc> effectDescs(scalingOptions.effects.size());
	std::atomic<bool> allSuccess = true;

	int duration = Utils::Measure([&]() {
		Win32Utils::RunParallel([&](uint32_t id) {
			std::optional<EffectDesc> desc = CompileEffect(scalingOptions, scalingOptions.effects[id]);
			if (desc) {
				effectDescs[id] = std::move(*desc);
			} else {
				allSuccess = false;
			}
		}, effectCount);
	});

	if (!allSuccess) {
		return {};
	}

	if (effectCount > 1) {
		Logger::Get().Info(fmt::format("编译着色器总计用时 {} 毫秒", duration / 1000.0f));
	}

	ID3D11Texture2D* effectInput = nullptr;

	/*DownscalingEffect& downscalingEffect = MagApp::Get().GetOptions().downscalingEffect;
	if (!downscalingEffect.name.empty()) {
		_effects.reserve(effectsOption.size() + 1);
	}*/
	effectDrawers.resize(scalingOptions.effects.size());

	for (uint32_t i = 0; i < effectCount; ++i) {
		bool isLastEffect = i == effectCount - 1;

		if (!effectDrawers[i].Initialize(
			effectDescs[i],
			scalingOptions.effects[i],
			effectInput,
			isLastEffect ? &outputRect : nullptr,
			isLastEffect ? &virtualOutputRect : nullptr
		)) {
			Logger::Get().Error(fmt::format("初始化效果#{} ({}) 失败", i, StrUtils::UTF16ToUTF8(scalingOptions.effects[i].name)));
			return {};
		}

		effectInput = effectDrawers[i].GetOutputTexture();
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

	return effectDrawers;
}

void Renderer::_BackendThreadProc(HWND hwndScaling, const ScalingOptions& options) noexcept {
	winrt::init_apartment(winrt::apartment_type::single_threaded);

	DeviceResources deviceResources;
	if (!deviceResources.Initialize(hwndScaling, options)) {
		return;
	}

	RECT outputRect;
	RECT virtualOutputRect;
	std::vector<EffectDrawer> effectDrawers = BuildEffects(options, outputRect, virtualOutputRect);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		DispatchMessage(&msg);
	}
}

}
