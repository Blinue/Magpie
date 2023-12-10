#pragma once
#include "SettingsExpander.g.h"

namespace winrt::Magpie::App::implementation {

struct SettingsExpander : SettingsExpander_base<SettingsExpander> {
	SettingsExpander();

	IInspectable Header() const {
		return GetValue(_headerProperty);
	}

	void Header(IInspectable const& value) {
		SetValue(_headerProperty, value);
	}

	IInspectable Description() const {
		return GetValue(_descriptionProperty);
	}

	void Description(IInspectable const& value) {
		SetValue(_descriptionProperty, value);
	}

	Controls::IconElement HeaderIcon() const {
		return GetValue(_headerIconProperty).as<Controls::IconElement>();
	}

	void HeaderIcon(Controls::IconElement const& value) {
		SetValue(_headerIconProperty, value);
	}

	IInspectable Content() const {
		return GetValue(_contentProperty);
	}

	void Content(IInspectable const& value) {
		SetValue(_contentProperty, value);
	}

	static DependencyProperty HeaderProperty() {
		return _headerProperty;
	}

	static DependencyProperty DescriptionProperty() {
		return _descriptionProperty;
	}

	static DependencyProperty HeaderIconProperty() {
		return _headerIconProperty;
	}

	static DependencyProperty ContentProperty() {
		return _contentProperty;
	}

private:
	static const DependencyProperty _headerProperty;
	static const DependencyProperty _descriptionProperty;
	static const DependencyProperty _headerIconProperty;
	static const DependencyProperty _contentProperty;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct SettingsExpander : SettingsExpanderT<SettingsExpander, implementation::SettingsExpander> {
};

}
