#include "pch.h"
#include "KeyVisual.h"
#if __has_include("KeyVisual.g.cpp")
#include "KeyVisual.g.cpp"
#endif

#include "StrUtils.h"
#include "XamlUtils.h"

using namespace winrt;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Markup;


namespace winrt::Magpie::App::implementation {

const DependencyProperty KeyVisual::ContentProperty = DependencyProperty::Register(
	L"Content",
	xaml_typename<IInspectable>(),
	xaml_typename<Magpie::App::KeyVisual>(),
	PropertyMetadata(box_value(L""), &KeyVisual::_OnPropertyChanged)
);

const DependencyProperty KeyVisual::VisualTypeProperty = DependencyProperty::Register(
	L"VisualTypeProperty",
	xaml_typename<IInspectable>(),
	xaml_typename<Magpie::App::KeyVisual>(),
	PropertyMetadata(box_value(Magpie::App::VisualType{}), &KeyVisual::_OnPropertyChanged)
);

const DependencyProperty KeyVisual::IsErrorProperty = DependencyProperty::Register(
	L"IsError",
	xaml_typename<bool>(),
	xaml_typename<Magpie::App::KeyVisual>(),
	PropertyMetadata(box_value(false), &KeyVisual::_OnIsErrorChanged)
);

KeyVisual::KeyVisual() {
	DefaultStyleKey(box_value(GetRuntimeClassName()));
	Style(_GetStyleSize(L"TextKeyVisualStyle"));
}

void KeyVisual::Content(IInspectable const& value) {
	SetValue(ContentProperty, box_value(value));
}

IInspectable KeyVisual::Content() const {
	return GetValue(ContentProperty);
}

void KeyVisual::VisualType(Magpie::App::VisualType value) {
	SetValue(VisualTypeProperty, box_value(value));
}

Magpie::App::VisualType KeyVisual::VisualType() const {
	return GetValue(VisualTypeProperty).as<Magpie::App::VisualType>();
}

void KeyVisual::IsError(bool value) {
	SetValue(IsErrorProperty, box_value(value));
}

bool KeyVisual::IsError() const {
	return GetValue(IsErrorProperty).as<bool>();
}

void KeyVisual::OnApplyTemplate() {
	if (_isEnabledChangedToken) {
		IsEnabledChanged(_isEnabledChangedToken);
	}

	_keyPresenter = GetTemplateChild(L"KeyPresenter").as<ContentPresenter>();

	_Update();
	_SetEnabledState();
	_SetErrorState();

	_isEnabledChangedToken = IsEnabledChanged({ this, &KeyVisual::_IsEnabledChanged });
	__super::OnApplyTemplate();
}

void KeyVisual::_OnPropertyChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	get_self<KeyVisual>(sender.as<default_interface<KeyVisual>>())->_Update();
}

void KeyVisual::_OnIsErrorChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	get_self<KeyVisual>(sender.as<default_interface<KeyVisual>>())->_SetErrorState();
}

void KeyVisual::_IsEnabledChanged(IInspectable const&, DependencyPropertyChangedEventArgs const&) {
	_SetEnabledState();
}

void KeyVisual::_Update() {
	if (!_keyPresenter) {
		return;
	}

	IInspectable content = Content();
	if (!content) {
		return;
	}
	
	if (content.as<IPropertyValue>().Type() == PropertyType::String) {
		Style(_GetStyleSize(L"TextKeyVisualStyle"));
		_keyPresenter.Content(content);
		return;
	}

	Style(_GetStyleSize(L"IconKeyVisualStyle"));

	int key = content.as<int>();
	switch (key) {
	case VK_UP:
		_keyPresenter.Content(box_value(L"\uE96D"));
		break;
	case VK_DOWN:
		_keyPresenter.Content(box_value(L"\uE96E"));
		break;
	case VK_LEFT:
		_keyPresenter.Content(box_value(L"\uE96F"));
		break;
	case VK_RIGHT:
		_keyPresenter.Content(box_value(L"\uE970"));
		break;
	case VK_LWIN:
	case VK_RWIN:
	{
		PathIcon winIcon = XamlReader::Load(LR"(<PathIcon xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" Data="M9,17V9h8v8ZM0,17V9H8v8ZM9,8V0h8V8ZM0,8V0H8V8Z" />)").as<PathIcon>();
		Viewbox winIconContainer;
		winIconContainer.Child(winIcon);
		winIconContainer.HorizontalAlignment(HorizontalAlignment::Center);
		winIconContainer.VerticalAlignment(VerticalAlignment::Center);

		double iconDimensions = _GetIconSize();
		winIconContainer.Height(iconDimensions);
		winIconContainer.Width(iconDimensions);
		_keyPresenter.Content(winIconContainer);
		break;
	}
	
	default: _keyPresenter.Content(box_value(L"")); break;
	}
}

Style KeyVisual::_GetStyleSize(std::wstring_view styleName) const {
	const wchar_t* prefix = nullptr;

	Magpie::App::VisualType vt = VisualType();
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

double KeyVisual::_GetIconSize() const {
	const wchar_t* key = nullptr;

	Magpie::App::VisualType vt = VisualType();
	if (vt == VisualType::Small || vt == VisualType::SmallOutline) {
		key = L"SmallIconSize";
	} else {
		key = L"DefaultIconSize";
	}

	return Application::Current().Resources()
		.Lookup(box_value(key))
		.as<double>();
}

void KeyVisual::_SetErrorState() {
	VisualStateManager::GoToState(*this, IsError() ? L"Error" : L"Default", true);
}

void KeyVisual::_SetEnabledState() {
	VisualStateManager::GoToState(*this, IsEnabled() ? L"Normal" : L"Disabled", true);
}

}
