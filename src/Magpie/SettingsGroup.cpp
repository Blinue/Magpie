#include "pch.h"
#include "SettingsGroup.h"
#if __has_include("SettingsGroup.g.cpp")
#include "SettingsGroup.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Data;

namespace winrt::Magpie::implementation {

// Header 如果为字符串类型会编译失败，见 https://github.com/microsoft/microsoft-ui-xaml/issues/5395
const DependencyProperty SettingsGroup::_headerProperty = DependencyProperty::Register(
	L"Header",
	xaml_typename<IInspectable>(),
	xaml_typename<class_type>(),
	nullptr
);

const DependencyProperty SettingsGroup::_descriptionProperty = DependencyProperty::Register(
	L"Description",
	xaml_typename<IInspectable>(),
	xaml_typename<class_type>(),
	PropertyMetadata(nullptr, &SettingsGroup::_OnDescriptionChanged)
);

void SettingsGroup::OnApplyTemplate() {
	base_type::OnApplyTemplate();

	_isEnabledChangedRevoker = IsEnabledChanged(auto_revoke, [this](IInspectable const&, DependencyPropertyChangedEventArgs const&) {
		_SetEnabledState();
	});
	_SetEnabledState();
}

void SettingsGroup::_OnDescriptionChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const& args) {
	SettingsGroup* that = get_self<SettingsGroup>(sender.as<class_type>());

	if (FrameworkElement descriptionPresenter = that->GetTemplateChild(L"DescriptionPresenter").try_as<FrameworkElement>()) {
		descriptionPresenter.Visibility(args.NewValue() == nullptr ? Visibility::Collapsed : Visibility::Visible);
	}
}

void SettingsGroup::_SetEnabledState() {
	VisualStateManager::GoToState(*this, IsEnabled() ? L"Normal" : L"Disabled", true);
}

}
