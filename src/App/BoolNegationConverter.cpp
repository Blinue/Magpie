#include "pch.h"
#include "BoolNegationConverter.h"
#if __has_include("BoolNegationConverter.g.cpp")
#include "BoolNegationConverter.g.cpp"
#endif


namespace winrt::Magpie::App::implementation {

IInspectable BoolNegationConverter::Convert(IInspectable const& value, Interop::TypeName const&, IInspectable const&, hstring const&) {
	return box_value(!unbox_value<bool>(value));
}

IInspectable BoolNegationConverter::ConvertBack(IInspectable const& value, Interop::TypeName const&, IInspectable const&, hstring const&) {
	return box_value(!unbox_value<bool>(value));
}

}
