#include "pch.h"
#include "KeyVisual.h"
#if __has_include("KeyVisual.g.cpp")
#include "KeyVisual.g.cpp"
#endif

#include "StrUtils.h"

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::UI::Xaml;


namespace winrt::Magpie::implementation {

const DependencyProperty KeyVisual::ContentProperty = DependencyProperty::Register(
	L"Content",
	xaml_typename<IInspectable>(),
	xaml_typename<Magpie::KeyVisual>(),
	PropertyMetadata(box_value(L""), &KeyVisual::_OnPropertyChanged)
);

const DependencyProperty KeyVisual::VisualTypeProperty = DependencyProperty::Register(
	L"VisualTypeProperty",
	xaml_typename<IInspectable>(),
	xaml_typename<Magpie::KeyVisual>(),
	PropertyMetadata(box_value(Magpie::VisualType{}), &KeyVisual::_OnPropertyChanged)
);

KeyVisual::KeyVisual() {
	DefaultStyleKey(box_value(name_of<Magpie::KeyVisual>()));
	Style(_GetStyleSize(L"TextKeyVisualStyle"));
}

void KeyVisual::Content(IInspectable const& value) {
	SetValue(ContentProperty, box_value(value));
}

IInspectable KeyVisual::Content() const {
	return GetValue(ContentProperty);
}

void KeyVisual::VisualType(Magpie::VisualType value) {
	SetValue(VisualTypeProperty, box_value(value));
}

Magpie::VisualType KeyVisual::VisualType() const {
	return GetValue(VisualTypeProperty).as<Magpie::VisualType>();
}

void KeyVisual::_OnPropertyChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	get_self<KeyVisual>(sender.as<default_interface<KeyVisual>>())->_Update();
}

void KeyVisual::_Update() {
}

Style KeyVisual::_GetStyleSize(std::wstring_view styleName) {
	const wchar_t* prefix = nullptr;

	Magpie::VisualType vt = VisualType();
	if (vt == VisualType::Small) {
		prefix = L"Small";
	} else if (vt == VisualType::SmallOutline) {
		prefix = L"SmallOutline";
	} else {
		prefix = L"Default";
	}

	return Application::Current().Resources()
		.Lookup(box_value(StrUtils::ConcatW(prefix, styleName)))
		.as<Windows::UI::Xaml::Style>();
}

}
