#include "pch.h"
#include "SettingsExpanderCornerRadiusConverter.h"
#if __has_include("SettingsExpanderCornerRadiusConverter.g.cpp")
#include "SettingsExpanderCornerRadiusConverter.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml::Interop;

namespace winrt::Magpie::App::implementation {

IInspectable SettingsExpanderCornerRadiusConverter::Convert(IInspectable const& value, TypeName const&, IInspectable const&, hstring const&) {
	auto cornerRadius = value.try_as<CornerRadius>();
	if (!cornerRadius) {
		return value;
	}

	cornerRadius->TopLeft = 0;
	cornerRadius->TopRight = 0;
	return box_value(*cornerRadius);
}

IInspectable SettingsExpanderCornerRadiusConverter::ConvertBack(IInspectable const& value, TypeName const&, IInspectable const&, hstring const&) {
	return value;
}

}
