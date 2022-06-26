#pragma once

#include "BooleanToVisibilityConverter.g.h"


namespace winrt::Magpie::App::implementation {

struct BooleanToVisibilityConverter : BooleanToVisibilityConverterT<BooleanToVisibilityConverter> {
    BooleanToVisibilityConverter() = default;

    IInspectable Convert(IInspectable const& value, Interop::TypeName const&, IInspectable const&, hstring const&);
    IInspectable ConvertBack(IInspectable const&, Interop::TypeName const&, IInspectable const&, hstring const&);
};

}

namespace winrt::Magpie::App::factory_implementation {

struct BooleanToVisibilityConverter : BooleanToVisibilityConverterT<BooleanToVisibilityConverter, implementation::BooleanToVisibilityConverter> {
};

}
