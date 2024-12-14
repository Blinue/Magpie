#pragma once
#include "EffectParametersViewModel.g.h"
#include "ScalingModeBoolParameter.g.h"
#include "ScalingModeFloatParameter.g.h"
#include <parallel_hashmap/phmap.h>

namespace Magpie {
struct EffectInfo;
}

namespace winrt::Magpie::implementation {

struct ScalingModeBoolParameter : ScalingModeBoolParameterT<ScalingModeBoolParameter>,
                                  wil::notify_property_changed_base<ScalingModeBoolParameter> {
	ScalingModeBoolParameter(uint32_t index, const hstring& label, bool initValue)
		: _index(index), _label(box_value(label)), _value(initValue) {
	}
	uint32_t Index() const noexcept {
		return _index;
	}

	bool Value() const noexcept {
		return _value;
	}

	void Value(bool value) {
		_value = value;
		RaisePropertyChanged(L"Value");
	}

	IInspectable Label() const noexcept {
		return _label;
	}

private:
	const uint32_t _index;
	IInspectable _label;
	bool _value;
};

struct ScalingModeFloatParameter : ScalingModeFloatParameterT<ScalingModeFloatParameter>,
                                   wil::notify_property_changed_base<ScalingModeFloatParameter> {
	ScalingModeFloatParameter(uint32_t index, const hstring& label, float initValue, float minimum, float maximum, float step)
		: _index(index), _label(label), _value(initValue), _minimum(minimum), _maximum(maximum), _step(step) {
	}

	uint32_t Index() const noexcept {
		return _index;
	}

	double Value() const noexcept {
		return _value;
	}

	void Value(double value) {
		_value = value;
		RaisePropertyChanged(L"Value");
		RaisePropertyChanged(L"ValueText");
	}

	hstring ValueText() const noexcept {
		return ScalingModesPage::NumberFormatter().FormatDouble(_value);
	}

	hstring Label() const noexcept {
		return _label;
	}

	double Minimum() const noexcept {
		return _minimum;
	}

	double Maximum() const noexcept {
		return _maximum;
	}

	double Step() const noexcept {
		return _step;
	}

private:
	const uint32_t _index;
	const hstring _label;
	const double _minimum;
	const double _maximum;
	const double _step;
	double _value;
};

struct EffectParametersViewModel : EffectParametersViewModelT<EffectParametersViewModel> {
	EffectParametersViewModel() : EffectParametersViewModel(
		std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint32_t>::max()) {}

	EffectParametersViewModel(uint32_t scalingModeIdx, uint32_t effectIdx);

	uint32_t ScalingModeIdx() const noexcept {
		return _scalingModeIdx;
	}

	void ScalingModeIdx(uint32_t value) noexcept {
		_scalingModeIdx = value;
	}

	uint32_t EffectIdx() const noexcept {
		return _effectIdx;
	}

	void EffectIdx(uint32_t value) noexcept {
		_effectIdx = value;
	}

	IVector<IInspectable> BoolParams() const noexcept {
		return _boolParams;
	}

	IVector<IInspectable> FloatParams() const noexcept {
		return _floatParams;
	}

	bool HasBoolParams() const noexcept {
		return _boolParams != nullptr;
	}

	bool HasFloatParams() const noexcept {
		return _floatParams != nullptr;
	}

private:
	void _ScalingModeBoolParameter_PropertyChanged(IInspectable const& sender, PropertyChangedEventArgs const& args);

	void _ScalingModeFloatParameter_PropertyChanged(IInspectable const& sender, PropertyChangedEventArgs const& args);

	phmap::flat_hash_map<std::wstring, float>& _Data();

	IVector<IInspectable> _boolParams{ nullptr };
	IVector<IInspectable> _floatParams{ nullptr };

	uint32_t _scalingModeIdx;
	uint32_t _effectIdx;
	const ::Magpie::EffectInfo* _effectInfo = nullptr;
};

}

namespace winrt::Magpie::factory_implementation {

struct ScalingModeBoolParameter : ScalingModeBoolParameterT<ScalingModeBoolParameter, implementation::ScalingModeBoolParameter> {
};

struct ScalingModeFloatParameter : ScalingModeFloatParameterT<ScalingModeFloatParameter, implementation::ScalingModeFloatParameter> {
};

struct EffectParametersViewModel : EffectParametersViewModelT<EffectParametersViewModel, implementation::EffectParametersViewModel> {
};

}
