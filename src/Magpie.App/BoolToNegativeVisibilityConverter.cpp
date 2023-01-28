#include "pch.h"
#include "BoolToNegativeVisibilityConverter.h"
#if __has_include("BoolToNegativeVisibilityConverter.g.cpp")
#include "BoolToNegativeVisibilityConverter.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml::Interop;

namespace winrt::Magpie::App::implementation {

IInspectable BoolToNegativeVisibilityConverter::Convert(IInspectable const& value, TypeName const&, IInspectable const&, hstring const&) {
    return box_value(unbox_value<bool>(value) ? Visibility::Collapsed : Visibility::Visible);
}

IInspectable BoolToNegativeVisibilityConverter::ConvertBack(IInspectable const& value, TypeName const&, IInspectable const&, hstring const&) {
    return box_value(unbox_value<Visibility>(value) == Visibility::Collapsed);
}

}
