#pragma once
#include "SettingsExpanderCornerRadiusConverter.g.h"

namespace winrt::Magpie::implementation {

struct SettingsExpanderCornerRadiusConverter : SettingsExpanderCornerRadiusConverterT<SettingsExpanderCornerRadiusConverter> {
    IInspectable Convert(IInspectable const& value, Interop::TypeName const&, IInspectable const&, hstring const&);
    IInspectable ConvertBack(IInspectable const& value, Interop::TypeName const&, IInspectable const&, hstring const&);
};

}

BASIC_FACTORY(SettingsExpanderCornerRadiusConverter)
