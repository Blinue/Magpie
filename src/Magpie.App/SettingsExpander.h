#pragma once
#include "SettingsExpander.g.h"

namespace winrt::Magpie::App::implementation {

struct SettingsExpander : SettingsExpander_base<SettingsExpander> {
	SettingsExpander();

	IInspectable Content() const {
		return GetValue(_contentProperty);
	}

	void Content(IInspectable const& value) {
		SetValue(_contentProperty, value);
	}

	static DependencyProperty ContentProperty() {
		return _contentProperty;
	}

private:
	static const DependencyProperty _contentProperty;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct SettingsExpander : SettingsExpanderT<SettingsExpander, implementation::SettingsExpander> {
};

}
