#pragma once
#include "pch.h"
#include "BoolToVisibilityConverter.g.h"


namespace winrt::Magpie::App::implementation {

struct BoolToVisibilityConverter : BoolToVisibilityConverterT<BoolToVisibilityConverter> {
    BoolToVisibilityConverter() = default;

    IInspectable Convert(IInspectable const& value, Interop::TypeName const&, IInspectable const&, hstring const&);
    IInspectable ConvertBack(IInspectable const& value, Interop::TypeName const&, IInspectable const&, hstring const&);
};

}

namespace winrt::Magpie::App::factory_implementation {

struct BoolToVisibilityConverter : BoolToVisibilityConverterT<BoolToVisibilityConverter, implementation::BoolToVisibilityConverter> {
};

}
