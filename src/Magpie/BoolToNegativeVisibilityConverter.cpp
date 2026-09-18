#include "pch.h"
#include "BoolToNegativeVisibilityConverter.h"
#if __has_include("BoolToNegativeVisibilityConverter.g.cpp")
#include "BoolToNegativeVisibilityConverter.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml::Interop;

namespace winrt::Magpie::implementation {

IInspectable BoolToNegativeVisibilityConverter::Convert(const IInspectable& value, const TypeName&, const IInspectable&, const hstring&) {
    return box_value(value.try_as<bool>().value() ? Visibility::Collapsed : Visibility::Visible);
}

IInspectable BoolToNegativeVisibilityConverter::ConvertBack(const IInspectable& value, const TypeName&, const IInspectable&, const hstring&) {
    return box_value(value.try_as<Visibility>().value() == Visibility::Collapsed);
}

}
