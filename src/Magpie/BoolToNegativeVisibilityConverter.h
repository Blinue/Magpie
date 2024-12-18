#pragma once
#include "BoolToNegativeVisibilityConverter.g.h"

namespace winrt::Magpie::implementation {

struct BoolToNegativeVisibilityConverter : BoolToNegativeVisibilityConverterT<BoolToNegativeVisibilityConverter> {
    IInspectable Convert(IInspectable const& value, Interop::TypeName const&, IInspectable const&, hstring const&);
    IInspectable ConvertBack(IInspectable const& value, Interop::TypeName const&, IInspectable const&, hstring const&);
};

}

namespace winrt::Magpie::factory_implementation {

struct BoolToNegativeVisibilityConverter : BoolToNegativeVisibilityConverterT<BoolToNegativeVisibilityConverter, implementation::BoolToNegativeVisibilityConverter> {
};

}
