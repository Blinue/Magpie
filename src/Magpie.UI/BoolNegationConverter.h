#pragma once
#include "pch.h"
#include "BoolNegationConverter.g.h"


namespace winrt::Magpie::UI::implementation {

struct BoolNegationConverter : BoolNegationConverterT<BoolNegationConverter> {
    BoolNegationConverter() = default;

    IInspectable Convert(IInspectable const& value, Interop::TypeName const&, IInspectable const&, hstring const&);
    IInspectable ConvertBack(IInspectable const& value, Interop::TypeName const&, IInspectable const&, hstring const&);
};

}

namespace winrt::Magpie::UI::factory_implementation {

struct BoolNegationConverter : BoolNegationConverterT<BoolNegationConverter, implementation::BoolNegationConverter> {
};

}
