#include "pch.h"
#include "SettingsExpander.h"
#if __has_include("SettingsExpander.g.cpp")
#include "SettingsExpander.g.cpp"
#endif

namespace winrt::Magpie::App::implementation {

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
