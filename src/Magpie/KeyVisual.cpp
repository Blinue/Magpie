#include "pch.h"
#include "KeyVisual.h"
#if __has_include("KeyVisual.g.cpp")
#include "KeyVisual.g.cpp"
#endif
#if __has_include("KeyVisualStyle.g.cpp")
#include "KeyVisualStyle.g.cpp"
#endif
#include "StrHelper.h"
#include "Win32Helper.h"

using namespace ::Magpie;
using namespace winrt;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Markup;

namespace winrt::Magpie::implementation {

KeyVisual::KeyVisual() {
	DefaultStyleKey(box_value(GetRuntimeClassName()));
	Style(_GetStyleSize(L"TextKeyVisualStyle"));
}

void KeyVisual::Key(int value) {
	if (value == _key) {
		return;
	}

	_key = value;
	RaisePropertyChanged(L"Key");

	_Update();
}

void KeyVisual::VisualType(winrt::Magpie::VisualType value) {
	if (value == _visualType) {
		return;
	}

	_visualType = value;
	RaisePropertyChanged(L"VisualType");

	_Update();
}

void KeyVisual::IsError(bool value) {
	if (value == _isError) {
		return;
	}

	_isError = value;
	RaisePropertyChanged(L"IsError");

	_SetErrorState();
}

void KeyVisual::OnApplyTemplate() {
	base_type::OnApplyTemplate();

	_isEnabledChangedRevoker.revoke();

	_keyPresenter = GetTemplateChild(L"KeyPresenter").as<ContentPresenter>();

	_Update();
	_SetEnabledState();
	_SetErrorState();

	_isEnabledChangedRevoker = IsEnabledChanged(auto_revoke, { this, &KeyVisual::_IsEnabledChanged });
}

void KeyVisual::_IsEnabledChanged(IInspectable const&, DependencyPropertyChangedEventArgs const&) {
	_SetEnabledState();
}

void KeyVisual::_Update() {
	if (!_keyPresenter) {
		return;
	}

	uint8_t key = (uint8_t)Key();
	switch (key) {
	case VK_UP:
		Style(_GetStyleSize(L"IconKeyVisualStyle"));
		_keyPresenter.Content(box_value(L"\uE96D"));
		break;
	case VK_DOWN:
		Style(_GetStyleSize(L"IconKeyVisualStyle"));
		_keyPresenter.Content(box_value(L"\uE96E"));
		break;
	case VK_LEFT:
		Style(_GetStyleSize(L"IconKeyVisualStyle"));
		_keyPresenter.Content(box_value(L"\uE96F"));
		break;
	case VK_RIGHT:
		Style(_GetStyleSize(L"IconKeyVisualStyle"));
		_keyPresenter.Content(box_value(L"\uE970"));
		break;
	case VK_LWIN:
	{
		Style(_GetStyleSize(L"IconKeyVisualStyle"));

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
	case VK_LCONTROL:
		Style(_GetStyleSize(L"TextKeyVisualStyle"));
		_keyPresenter.Content(box_value(L"Ctrl"));
		break;
	case VK_LMENU:
		Style(_GetStyleSize(L"TextKeyVisualStyle"));
		_keyPresenter.Content(box_value(L"Alt"));
		break;
	case VK_LSHIFT:
		Style(_GetStyleSize(L"TextKeyVisualStyle"));
		_keyPresenter.Content(box_value(L"Shift"));
		break;
	default:
		Style(_GetStyleSize(L"TextKeyVisualStyle"));
		_keyPresenter.Content(box_value(Win32Helper::GetKeyName(key)));
		break;
	}
}

Style KeyVisual::_GetStyleSize(std::wstring_view styleName) const {
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
		.Lookup(box_value(StrHelper::Concat(prefix, styleName)))
		.as<Windows::UI::Xaml::Style>();
}

double KeyVisual::_GetIconSize() const {
	const wchar_t* key = nullptr;

	Magpie::VisualType vt = VisualType();
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
