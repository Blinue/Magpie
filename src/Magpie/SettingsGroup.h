#pragma once
#include "SettingsGroup.g.h"

namespace winrt::Magpie::implementation {

struct SettingsGroup : SettingsGroupT<SettingsGroup> {
	DEFINE_DEPENDENCY_PROPERTY(hstring, Header, _headerProperty)
	DEFINE_DEPENDENCY_PROPERTY(IInspectable, Description, _descriptionProperty)

public:
	SettingsGroup();

	void OnApplyTemplate();

private:
	static void _RegisterDependencyProperties();

	static void _OnDescriptionChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);

	void _SetEnabledState();

	IsEnabledChanged_revoker _isEnabledChangedRevoker;
};

}

BASIC_FACTORY(SettingsGroup)
