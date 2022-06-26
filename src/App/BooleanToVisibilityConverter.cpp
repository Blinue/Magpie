#include "pch.h"
#include "BooleanToVisibilityConverter.h"
#if __has_include("BooleanToVisibilityConverter.g.cpp")
#include "BooleanToVisibilityConverter.g.cpp"
#endif


namespace winrt::Magpie::App::implementation {

IInspectable BooleanToVisibilityConverter::Convert(IInspectable const& value, Interop::TypeName const&, IInspectable const&, hstring const&) {
    return box_value(unbox_value<bool>(value) ? Visibility::Visible : Visibility::Collapsed);
}

IInspectable BooleanToVisibilityConverter::ConvertBack(IInspectable const&, Interop::TypeName const&, IInspectable const&, hstring const&) {
    throw hresult_not_implemented();
}

}
