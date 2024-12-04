#pragma once
#include "SettingsGroup.g.h"

namespace winrt::Magpie::implementation {

struct SettingsGroup : SettingsGroupT<SettingsGroup> {
	static DependencyProperty HeaderProperty() { return _headerProperty; }
	static DependencyProperty DescriptionProperty() { return _descriptionProperty; }

	void Header(IInspectable const& value) const { SetValue(_headerProperty, value); }
	IInspectable Header() const { return GetValue(_headerProperty); }

	void Description(IInspectable value) const { SetValue(_descriptionProperty, value); }
	IInspectable Description() const { return GetValue(_descriptionProperty); }

	void OnApplyTemplate();

private:
	static const DependencyProperty _childrenProperty;
	static const DependencyProperty _headerProperty;
	static const DependencyProperty _descriptionProperty;

	static void _OnDescriptionChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);

	void _SetEnabledState();

	IsEnabledChanged_revoker _isEnabledChangedRevoker;
};

}

namespace winrt::Magpie::factory_implementation {

struct SettingsGroup : SettingsGroupT<SettingsGroup, implementation::SettingsGroup> {
};

}
