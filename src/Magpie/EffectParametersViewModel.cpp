#include "pch.h"
#include "EffectParametersViewModel.h"
#if __has_include("ScalingModeBoolParameter.g.cpp")
#include "ScalingModeBoolParameter.g.cpp"
#endif
#if __has_include("ScalingModeFloatParameter.g.cpp")
#include "ScalingModeFloatParameter.g.cpp"
#endif
#if __has_include("EffectParametersViewModel.g.cpp")
#include "EffectParametersViewModel.g.cpp"
#endif
#include "StrHelper.h"
#include "AppSettings.h"
#include "ScalingModesService.h"
#include "ScalingMode.h"
#include "EffectsService.h"
#include "EffectDesc.h"
#include "App.h"

using namespace Magpie;

namespace winrt::Magpie::implementation {

// 限制保存频率
// 1 秒内没有新的调用才执行保存
static fire_and_forget LazySaveAppSettings() {
	using namespace std::chrono;

	static steady_clock::time_point lastInvokeTime;
	static bool sleeping = false;

	lastInvokeTime = steady_clock::now();

	if (sleeping) {
		co_return;
	}

	sleeping = true;
	
	co_await 1s;

	while (true) {
		int duration = (int)duration_cast<milliseconds>(steady_clock::now() - lastInvokeTime).count();
		if (duration >= 999) {
			break;
		}

		// 如果与 lastInvokeTime 相差不到 1s，则继续等待
		co_await milliseconds(1000 - duration);
	}

	// 回到主线程
	co_await App::Get().Dispatcher();

	sleeping = false;
	AppSettings::Get().SaveAsync();
}

EffectParametersViewModel::EffectParametersViewModel(uint32_t scalingModeIdx, uint32_t effectIdx)
	: _scalingModeIdx(scalingModeIdx), _effectIdx(effectIdx)
{
	ScalingMode& scalingMode = ScalingModesService::Get().GetScalingMode(_scalingModeIdx);
	_effectInfo = EffectsService::Get().GetEffect(scalingMode.effects[_effectIdx].name);

	phmap::flat_hash_map<std::wstring, float>& params = _Data();

	std::vector<IInspectable> boolParams;
	std::vector<IInspectable> floatParams;
	for (uint32_t i = 0, size = (uint32_t)_effectInfo->params.size(); i < size; ++i) {
		const EffectParameterDesc& param = _effectInfo->params[i];

		std::optional<float> paramValue;
		{
			auto it = params.find(StrHelper::UTF8ToUTF16(param.name));
			if (it != params.end()) {
				paramValue = it->second;
			}
		}
		
		if (param.constant.index() == 0) {
			const EffectConstant<float>& constant = std::get<0>(param.constant);
			auto floatParamItem = make_self<ScalingModeFloatParameter>(
				i,
				hstring(StrHelper::UTF8ToUTF16(param.label.empty() ? param.name : param.label)),
				paramValue.has_value() ? *paramValue : constant.defaultValue,
				constant.minValue,
				constant.maxValue,
				constant.step
			);
			floatParamItem->PropertyChanged({ this, &EffectParametersViewModel::_ScalingModeFloatParameter_PropertyChanged });
			floatParams.push_back(*floatParamItem);
		} else {
			const EffectConstant<int>& constant = std::get<1>(param.constant);
			if (constant.minValue == 0 && constant.maxValue == 1 && constant.step == 1) {
				auto boolParamItem = make_self<ScalingModeBoolParameter>(
					i,
					hstring(StrHelper::UTF8ToUTF16(param.label.empty() ? param.name : param.label)),
					paramValue.has_value() ? std::abs(*paramValue) > 1e-6 : (bool)constant.defaultValue
				);
				boolParamItem->PropertyChanged({ this, &EffectParametersViewModel::_ScalingModeBoolParameter_PropertyChanged });
				boolParams.push_back(*boolParamItem);
			} else {
				auto floatParamItem = make_self<ScalingModeFloatParameter>(
					i,
					hstring(StrHelper::UTF8ToUTF16(param.label.empty() ? param.name : param.label)),
					paramValue.has_value() ? *paramValue : (float)constant.defaultValue,
					(float)constant.minValue,
					(float)constant.maxValue,
					(float)constant.step
				);
				floatParamItem->PropertyChanged({ this, &EffectParametersViewModel::_ScalingModeFloatParameter_PropertyChanged });
				floatParams.push_back(*floatParamItem);
			}
		}
	}
	if (!boolParams.empty()) {
		_boolParams = single_threaded_vector(std::move(boolParams));
	}
	if (!floatParams.empty()) {
		_floatParams = single_threaded_vector(std::move(floatParams));
	}
}

void EffectParametersViewModel::_ScalingModeBoolParameter_PropertyChanged(
	IInspectable const& sender,
	PropertyChangedEventArgs const& args
) {
	if (args.PropertyName() != L"Value") {
		return;
	}

	ScalingModeBoolParameter* boolParamImpl =
		get_self<ScalingModeBoolParameter>(sender.as<Magpie::ScalingModeBoolParameter>());
	const std::string& effectName = _effectInfo->params[boolParamImpl->Index()].name;
	_Data()[StrHelper::UTF8ToUTF16(effectName)] = (float)boolParamImpl->Value();

	LazySaveAppSettings();
}

void EffectParametersViewModel::_ScalingModeFloatParameter_PropertyChanged(
	IInspectable const& sender,
	PropertyChangedEventArgs const& args
) {
	if (args.PropertyName() != L"Value") {
		return;
	}

	ScalingModeFloatParameter* floatParamImpl =
		get_self<ScalingModeFloatParameter>(sender.as<Magpie::ScalingModeFloatParameter>());
	const std::string& effectName = _effectInfo->params[floatParamImpl->Index()].name;
	_Data()[StrHelper::UTF8ToUTF16(effectName)] = (float)floatParamImpl->Value();

	LazySaveAppSettings();
}

phmap::flat_hash_map<std::wstring, float>& EffectParametersViewModel::_Data() {
	ScalingMode& scalingMode = ScalingModesService::Get().GetScalingMode(_scalingModeIdx);
	return scalingMode.effects[_effectIdx].parameters;
}

}
