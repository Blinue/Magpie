#pragma once
#include "TitleBarControl.g.h"

namespace winrt::Magpie::App::implementation {
struct TitleBarControl : TitleBarControlT<TitleBarControl> {
	TitleBarControl();

	void Loading(FrameworkElement const&, IInspectable const&);

	Imaging::SoftwareBitmapSource Logo() const noexcept {
		return _logo;
	}

	event_token PropertyChanged(PropertyChangedEventHandler const& value) {
		return _propertyChangedEvent.add(value);
	}

	void PropertyChanged(event_token const& token) {
		_propertyChangedEvent.remove(token);
	}

	void IsWindowActive(bool value);

private:
	Imaging::SoftwareBitmapSource _logo{ nullptr };
	event<PropertyChangedEventHandler> _propertyChangedEvent;
};
}

namespace winrt::Magpie::App::factory_implementation {

struct TitleBarControl : TitleBarControlT<TitleBarControl, implementation::TitleBarControl> {
};

}
