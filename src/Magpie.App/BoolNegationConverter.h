#pragma once
#include "BoolNegationConverter.g.h"

namespace winrt::Magpie::App::implementation {

struct BoolNegationConverter : BoolNegationConverterT<BoolNegationConverter> {
    BoolNegationConverter() = default;

    IInspectable Convert(IInspectable const& value, Interop::TypeName const&, IInspectable const&, hstring const&);
    IInspectable ConvertBack(IInspectable const& value, Interop::TypeName const&, IInspectable const&, hstring const&);
};

}

namespace winrt::Magpie::App::factory_implementation {

struct BoolNegationConverter : BoolNegationConverterT<BoolNegationConverter, implementation::BoolNegationConverter> {
};

}
