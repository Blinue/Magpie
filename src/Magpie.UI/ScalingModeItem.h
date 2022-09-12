#pragma once
#include "ScalingModeItem.g.h"


namespace winrt::Magpie::UI {
struct ScalingMode;
}

namespace winrt::Magpie::UI::implementation {

struct ScalingModeItem : ScalingModeItemT<ScalingModeItem> {
	ScalingModeItem(uint32_t index);

	event_token PropertyChanged(PropertyChangedEventHandler const& handler) {
		return _propertyChangedEvent.add(handler);
	}

	void PropertyChanged(event_token const& token) noexcept {
		_propertyChangedEvent.remove(token);
	}

	hstring Name() const noexcept;

	void Name(const hstring& value) noexcept;

	hstring Description() const noexcept;

	IObservableVector<IInspectable> Effects() const noexcept {
		return _effects;
	}

private:
	event<PropertyChangedEventHandler> _propertyChangedEvent;
	ScalingMode* _scalingMode = nullptr;
	IObservableVector<IInspectable> _effects{ nullptr };
};

}

namespace winrt::Magpie::UI::factory_implementation {

struct ScalingModeItem : ScalingModeItemT<ScalingModeItem, implementation::ScalingModeItem> {
};

}
