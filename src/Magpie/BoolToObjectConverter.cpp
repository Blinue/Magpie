#include "pch.h"
#include "BoolToObjectConverter.h"
#if __has_include("BoolToObjectConverter.g.cpp")
#include "BoolToObjectConverter.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml::Interop;

namespace winrt::Magpie::implementation {

DependencyProperty BoolToObjectConverter::_trueObjectProperty{ nullptr };
DependencyProperty BoolToObjectConverter::_falseObjectProperty{ nullptr };

BoolToObjectConverter::BoolToObjectConverter() {
	_RegisterDependencyProperties();
}

IInspectable BoolToObjectConverter::Convert(IInspectable const& value, TypeName const&, IInspectable const&, hstring const&) {
	assert(TrueObject() && FalseObject());
	return box_value(value.try_as<bool>().value() ? TrueObject() : FalseObject());
}

IInspectable BoolToObjectConverter::ConvertBack(IInspectable const&, TypeName const&, IInspectable const&, hstring const&) {
	assert(false);
	return nullptr;
}

void BoolToObjectConverter::_RegisterDependencyProperties() {
	if (_trueObjectProperty) {
		return;
	}

	_trueObjectProperty = DependencyProperty::Register(
		L"TrueObject",
		xaml_typename<IInspectable>(),
		xaml_typename<class_type>(),
		nullptr
	);

	_falseObjectProperty = DependencyProperty::Register(
		L"FalseObject",
		xaml_typename<IInspectable>(),
		xaml_typename<class_type>(),
		nullptr
	);
}

}
