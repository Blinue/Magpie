#pragma once
#include "ScalingModeEffectItem.g.h"
#include "ScalingModeBoolParameter.g.h"
#include "ScalingModeFloatParameter.g.h"


namespace Magpie::Core {
struct EffectOption;
}

namespace winrt::Magpie::UI {
struct EffectInfo;
}

namespace winrt::Magpie::UI::implementation {

struct ScalingModeBoolParameter : ScalingModeBoolParameterT<ScalingModeBoolParameter> {
	ScalingModeBoolParameter(uint32_t index, const hstring& label, bool initValue);

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
	ScalingModeFloatParameter(uint32_t index, const hstring& label, float initValue, float minimum, float maximum, float step);

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

	hstring ValueText() const noexcept;

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

struct ScalingModeEffectItem : ScalingModeEffectItemT<ScalingModeEffectItem> {
	ScalingModeEffectItem(uint32_t scalingModeIdx, uint32_t effectIdx);

	event_token PropertyChanged(PropertyChangedEventHandler const& handler) {
		return _propertyChangedEvent.add(handler);
	}

	void PropertyChanged(event_token const& token) noexcept {
		_propertyChangedEvent.remove(token);
	}

	hstring Name() const noexcept {
		return hstring(_name);
	}

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

	bool CanScale() const noexcept;

	bool HasParameters() const noexcept;

	int ScalingType() const noexcept;

	void ScalingType(int value);

	bool IsShowScalingFactors() const noexcept;

	bool IsShowScalingPixels() const noexcept;

	double ScalingFactorX() const noexcept;

	void ScalingFactorX(double value);

	double ScalingFactorY() const noexcept;

	void ScalingFactorY(double value);

	double ScalingPixelsX() const noexcept;

	void ScalingPixelsX(double value);

	double ScalingPixelsY() const noexcept;

	void ScalingPixelsY(double value);

	IVector<IInspectable> BoolParams() const noexcept {
		return _boolParams;
	}

	IVector<IInspectable> FloatParams() const noexcept {
		return _floatParams;
	}

	void Remove();

	event_token Removed(EventHandler<uint32_t> const& handler) {
		return _removedEvent.add(handler);
	}

	void Removed(event_token const& token) noexcept {
		_removedEvent.remove(token);
	}

private:
	::Magpie::Core::EffectOption& _Data() noexcept;
	const ::Magpie::Core::EffectOption& _Data() const noexcept;

	event<PropertyChangedEventHandler> _propertyChangedEvent;

	uint32_t _scalingModeIdx = 0;
	uint32_t _effectIdx = 0;
	hstring _name;
	const EffectInfo* _effectInfo = nullptr;
	event<EventHandler<uint32_t>> _removedEvent;

	IVector<IInspectable> _boolParams{ nullptr };
	IVector<IInspectable> _floatParams{ nullptr };
};

}

namespace winrt::Magpie::UI::factory_implementation {

struct ScalingModeBoolParameter : ScalingModeBoolParameterT<ScalingModeBoolParameter, implementation::ScalingModeBoolParameter> {
};

struct ScalingModeFloatParameter : ScalingModeFloatParameterT<ScalingModeFloatParameter, implementation::ScalingModeFloatParameter> {
};

struct ScalingModeEffectItem : ScalingModeEffectItemT<ScalingModeEffectItem, implementation::ScalingModeEffectItem> {
};

}
