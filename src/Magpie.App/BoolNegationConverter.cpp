#include "pch.h"
#include "BoolNegationConverter.h"
#if __has_include("BoolNegationConverter.g.cpp")
#include "BoolNegationConverter.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml::Interop;

namespace winrt::Magpie::App::implementation {

IInspectable BoolNegationConverter::Convert(IInspectable const& value, TypeName const&, IInspectable const&, hstring const&) {
	return box_value(!unbox_value<bool>(value));
}

IInspectable BoolNegationConverter::ConvertBack(IInspectable const& value, TypeName const&, IInspectable const&, hstring const&) {
	return box_value(!unbox_value<bool>(value));
}

}
