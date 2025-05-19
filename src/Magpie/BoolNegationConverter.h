#pragma once
#include "BoolNegationConverter.g.h"

namespace winrt::Magpie::implementation {

struct BoolNegationConverter : BoolNegationConverterT<BoolNegationConverter> {
    IInspectable Convert(IInspectable const& value, Interop::TypeName const&, IInspectable const&, hstring const&);
    IInspectable ConvertBack(IInspectable const& value, Interop::TypeName const&, IInspectable const&, hstring const&);
};

}

BASIC_FACTORY(BoolNegationConverter)
