﻿#pragma once
#include "SettingsExpanderCornerRadiusConverter.g.h"

namespace winrt::Magpie::App::implementation {

struct SettingsExpanderCornerRadiusConverter : SettingsExpanderCornerRadiusConverterT<SettingsExpanderCornerRadiusConverter> {
    IInspectable Convert(IInspectable const& value, Interop::TypeName const&, IInspectable const&, hstring const&);
    IInspectable ConvertBack(IInspectable const& value, Interop::TypeName const&, IInspectable const&, hstring const&);
};

}

namespace winrt::Magpie::App::factory_implementation {

struct SettingsExpanderCornerRadiusConverter : SettingsExpanderCornerRadiusConverterT<SettingsExpanderCornerRadiusConverter, implementation::SettingsExpanderCornerRadiusConverter> {
};

}
