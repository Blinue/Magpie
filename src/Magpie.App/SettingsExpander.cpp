// 移植自 https://github.com/CommunityToolkit/Windows/tree/main/components/SettingsControls/src/SettingsExpander

#include "pch.h"
#include "SettingsExpander.h"
#if __has_include("SettingsExpander.g.cpp")
#include "SettingsExpander.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml::Controls;

namespace winrt::Magpie::App::implementation {

const DependencyProperty SettingsExpander::_headerProperty = DependencyProperty::Register(
	L"Header",
	xaml_typename<IInspectable>(),
	xaml_typename<Magpie::App::SettingsExpander>(),
	nullptr
);

const DependencyProperty SettingsExpander::_descriptionProperty = DependencyProperty::Register(
	L"Description",
	xaml_typename<IInspectable>(),
	xaml_typename<Magpie::App::SettingsExpander>(),
	nullptr
);

const DependencyProperty SettingsExpander::_headerIconProperty = DependencyProperty::Register(
	L"HeaderIcon",
	xaml_typename<IconElement>(),
	xaml_typename<Magpie::App::SettingsExpander>(),
	nullptr
);

const DependencyProperty SettingsExpander::_contentProperty = DependencyProperty::Register(
	L"Content",
	xaml_typename<IInspectable>(),
	xaml_typename<Magpie::App::SettingsExpander>(),
	nullptr
);

SettingsExpander::SettingsExpander() {
	DefaultStyleKey(box_value(GetRuntimeClassName()));
}

}
