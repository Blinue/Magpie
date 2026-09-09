#pragma once
#include "BoolToNegativeVisibilityConverter.g.h"

namespace winrt::Magpie::implementation {

struct BoolToNegativeVisibilityConverter : BoolToNegativeVisibilityConverterT<BoolToNegativeVisibilityConverter> {
    IInspectable Convert(const IInspectable& value, const Interop::TypeName&, const IInspectable&, const hstring&);
    IInspectable ConvertBack(const IInspectable& value, const Interop::TypeName&, const IInspectable&, const hstring&);
};

}

BASIC_FACTORY(BoolToNegativeVisibilityConverter)
