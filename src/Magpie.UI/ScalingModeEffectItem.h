#pragma once
#include "ScalingModeEffectItem.g.h"


namespace Magpie::Core {
struct EffectOption;
}

namespace winrt::Magpie::UI::implementation {

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

private:
	::Magpie::Core::EffectOption& _Data() noexcept;
	const ::Magpie::Core::EffectOption& _Data() const noexcept;

	event<PropertyChangedEventHandler> _propertyChangedEvent;

	uint32_t _scalingModeIdx = 0;
	uint32_t _effectIdx = 0;
	std::wstring_view _name;
};

}

namespace winrt::Magpie::UI::factory_implementation {

struct ScalingModeEffectItem : ScalingModeEffectItemT<ScalingModeEffectItem, implementation::ScalingModeEffectItem> {
};

}
