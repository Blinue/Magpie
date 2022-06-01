#pragma once

#include "winrt/Windows.UI.Xaml.h"
#include "winrt/Windows.UI.Xaml.Markup.h"
#include "winrt/Windows.UI.Xaml.Interop.h"
#include "winrt/Windows.UI.Xaml.Controls.Primitives.h"
#include "SettingsGroup1.g.h"

namespace winrt::Magpie::implementation
{
    struct SettingsGroup1 : SettingsGroup1T<SettingsGroup1>
    {
        SettingsGroup1();

        Windows::UI::Xaml::Controls::UIElementCollection Children() const;
        void Children(Windows::UI::Xaml::Controls::UIElementCollection const& value);

        static const Windows::UI::Xaml::DependencyProperty ChildrenProperty;
    };
}

namespace winrt::Magpie::factory_implementation
{
    struct SettingsGroup1 : SettingsGroup1T<SettingsGroup1, implementation::SettingsGroup1>
    {
    };
}
