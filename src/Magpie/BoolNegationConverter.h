#pragma once
#include "BoolNegationConverter.g.h"

namespace winrt::Magpie::implementation {

struct BoolNegationConverter : BoolNegationConverterT<BoolNegationConverter> {
    IInspectable Convert(const IInspectable& value, const Interop::TypeName&, const IInspectable&, const hstring&);
    IInspectable ConvertBack(const IInspectable& value, const Interop::TypeName&, const IInspectable&, const hstring&);
};

}

BASIC_FACTORY(BoolNegationConverter)
