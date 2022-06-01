#include "pch.h"
#include "SettingsGroup1.h"
#if __has_include("SettingsGroup1.g.cpp")
#include "SettingsGroup1.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;


namespace winrt::Magpie::implementation {

const DependencyProperty SettingsGroup1::ChildrenProperty = DependencyProperty::Register(
    L"Children",
    xaml_typename<UIElementCollection>(),
    xaml_typename<Magpie::SettingsGroup1>(),
    PropertyMetadata(nullptr)
);

SettingsGroup1::SettingsGroup1() {
    InitializeComponent();

    Children(PART_Host().Children());
}

UIElementCollection SettingsGroup1::Children() const {
    return GetValue(ChildrenProperty).as<UIElementCollection>();
}

void SettingsGroup1::Children(UIElementCollection const& value) {
    SetValue(ChildrenProperty, value);
}

}
