#pragma once
#include "EffectParametersViewModel.g.h"
#include "ScalingModeBoolParameter.g.h"
#include "ScalingModeFloatParameter.g.h"
#include <parallel_hashmap/phmap.h>
#include "EffectsService.h"


namespace winrt::Magpie::UI::implementation {

struct ScalingModeBoolParameter : ScalingModeBoolParameterT<ScalingModeBoolParameter> {
	ScalingModeBoolParameter(uint32_t index, const hstring& label, bool initValue)
		: _index(index), _label(label), _value(initValue) {
	}

	event_token PropertyChanged(PropertyChangedEventHandler const& handler) {
		return _propertyChangedEvent.add(handler);
	}

	void PropertyChanged(event_token const& token) noexcept {
		_propertyChangedEvent.remove(token);
	}

	uint32_t Index() const noexcept {
		return _index;
	}

	bool Value() const noexcept {
		return _value;
	}

	void Value(bool value) {
		_value = value;
		_propertyChangedEvent(*this, PropertyChangedEventArgs(L"Value"));
	}

	hstring Label() const noexcept {
		return _label;
	}

private:
	event<PropertyChangedEventHandler> _propertyChangedEvent;

	const uint32_t _index;
	const hstring _label;
	bool _value;
};

struct ScalingModeFloatParameter : ScalingModeFloatParameterT<ScalingModeFloatParameter> {
	ScalingModeFloatParameter(uint32_t index, const hstring& label, float initValue, float minimum, float maximum, float step)
		: _index(index), _label(label), _value(initValue), _minimum(minimum), _maximum(maximum), _step(step) {
	}

	event_token PropertyChanged(PropertyChangedEventHandler const& handler) {
		return _propertyChangedEvent.add(handler);
	}

	void PropertyChanged(event_token const& token) noexcept {
		_propertyChangedEvent.remove(token);
	}

	uint32_t Index() const noexcept {
		return _index;
	}

	double Value() const noexcept {
		return _value;
	}

	void Value(double value) {
		_value = value;
		_propertyChangedEvent(*this, PropertyChangedEventArgs(L"Value"));
		_propertyChangedEvent(*this, PropertyChangedEventArgs(L"ValueText"));
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
	event<PropertyChangedEventHandler> _propertyChangedEvent;

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

	bool _IsDefaultDownscalingEffect() const noexcept {
		return _scalingModeIdx == std::numeric_limits<uint32_t>::max();
	}

	IVector<IInspectable> _boolParams{ nullptr };
	IVector<IInspectable> _floatParams{ nullptr };

	uint32_t _scalingModeIdx;
	uint32_t _effectIdx;
	const EffectInfo* _effectInfo = nullptr;
};

}

namespace winrt::Magpie::UI::factory_implementation {

struct ScalingModeBoolParameter : ScalingModeBoolParameterT<ScalingModeBoolParameter, implementation::ScalingModeBoolParameter> {
};

struct ScalingModeFloatParameter : ScalingModeFloatParameterT<ScalingModeFloatParameter, implementation::ScalingModeFloatParameter> {
};

struct EffectParametersViewModel : EffectParametersViewModelT<EffectParametersViewModel, implementation::EffectParametersViewModel> {
};

}
