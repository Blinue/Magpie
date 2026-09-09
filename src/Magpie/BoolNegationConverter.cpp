#include "pch.h"
#include "BoolNegationConverter.h"
#if __has_include("BoolNegationConverter.g.cpp")
#include "BoolNegationConverter.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml::Interop;

namespace winrt::Magpie::implementation {

static IInspectable ConvertValue(const IInspectable& value) {
	return box_value(!value.try_as<bool>().value());
}

IInspectable BoolNegationConverter::Convert(const IInspectable& value, const TypeName&, const IInspectable&, const hstring&) {
	return ConvertValue(value);
}

IInspectable BoolNegationConverter::ConvertBack(const IInspectable& value, const TypeName&, const IInspectable&, const hstring&) {
	return ConvertValue(value);
}

}
