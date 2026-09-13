#include "pch.h"
#include "SettingsGroup.h"
#if __has_include("SettingsGroup.g.cpp")
#include "SettingsGroup.g.cpp"
#endif
#include "XamlHelper.h"

using namespace Magpie;
using namespace winrt;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Data;

namespace winrt::Magpie::implementation {

DependencyProperty SettingsGroup::_headerProperty{ nullptr };
DependencyProperty SettingsGroup::_descriptionProperty{ nullptr };

SettingsGroup::SettingsGroup() {
	_RegisterDependencyProperties();
}

void SettingsGroup::OnApplyTemplate() {
	base_type::OnApplyTemplate();

	_isEnabledChangedRevoker = IsEnabledChanged(auto_revoke, [this](const auto&, const auto&) {
		_SetEnabledState();
	});
	_SetEnabledState();
}

void SettingsGroup::_RegisterDependencyProperties() {
	if (_headerProperty) {
		return;
	}

	_headerProperty = DependencyProperty::Register(
		L"Header",
		xaml_typename<hstring>(),
		xaml_typename<class_type>(),
		nullptr
	);

	_descriptionProperty = DependencyProperty::Register(
		L"Description",
		xaml_typename<IInspectable>(),
		xaml_typename<class_type>(),
		PropertyMetadata(nullptr, &SettingsGroup::_OnDescriptionChanged)
	);
}

void SettingsGroup::_OnDescriptionChanged(
	DependencyObject const& sender,
	DependencyPropertyChangedEventArgs const& args
) {
	SettingsGroup* that = get_self<SettingsGroup>(sender.try_as<class_type>());

	if (FrameworkElement descriptionPresenter =
		that->GetTemplateChild(L"DescriptionPresenter").try_as<FrameworkElement>()) {
		descriptionPresenter.Visibility(
			XamlHelper::IsNullOrEmptyString(args.NewValue()) ? Visibility::Collapsed : Visibility::Visible);
	}
}

void SettingsGroup::_SetEnabledState() {
	VisualStateManager::GoToState(*this, IsEnabled() ? L"Normal" : L"Disabled", true);
}

}
