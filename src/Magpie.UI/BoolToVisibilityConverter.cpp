#include "pch.h"
#include "BoolToVisibilityConverter.h"
#if __has_include("BoolToVisibilityConverter.g.cpp")
#include "BoolToVisibilityConverter.g.cpp"
#endif


namespace winrt::Magpie::UI::implementation {

IInspectable BoolToVisibilityConverter::Convert(IInspectable const& value, Interop::TypeName const&, IInspectable const&, hstring const&) {
    return box_value(unbox_value<bool>(value) ? Visibility::Visible : Visibility::Collapsed);
}

IInspectable BoolToVisibilityConverter::ConvertBack(IInspectable const& value, Interop::TypeName const&, IInspectable const&, hstring const&) {
    return box_value(unbox_value<Visibility>(value) == Visibility::Visible);
}

}
