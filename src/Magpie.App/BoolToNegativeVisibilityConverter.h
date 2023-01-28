#pragma once
#include "BoolToNegativeVisibilityConverter.g.h"

namespace winrt::Magpie::App::implementation {

struct BoolToNegativeVisibilityConverter : BoolToNegativeVisibilityConverterT<BoolToNegativeVisibilityConverter> {
    BoolToNegativeVisibilityConverter() = default;

    IInspectable Convert(IInspectable const& value, Interop::TypeName const&, IInspectable const&, hstring const&);
    IInspectable ConvertBack(IInspectable const& value, Interop::TypeName const&, IInspectable const&, hstring const&);
};

}

namespace winrt::Magpie::App::factory_implementation {

struct BoolToNegativeVisibilityConverter : BoolToNegativeVisibilityConverterT<BoolToNegativeVisibilityConverter, implementation::BoolToNegativeVisibilityConverter> {
};

}
