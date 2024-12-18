#pragma once
#include "KeyVisual.g.h"

namespace winrt::Magpie::implementation {

struct KeyVisual : KeyVisualT<KeyVisual>, wil::notify_property_changed_base<KeyVisual> {
	KeyVisual();

	int Key() const noexcept { return _key; }
	void Key(int value);

	winrt::Magpie::VisualType VisualType() const noexcept { return _visualType; }
	void VisualType(winrt::Magpie::VisualType value);

	bool IsError() const noexcept { return _isError; }
	void IsError(bool value);

	void OnApplyTemplate();

private:
	void _IsEnabledChanged(IInspectable const&, DependencyPropertyChangedEventArgs const&);

	void _Update();
	Windows::UI::Xaml::Style _GetStyleSize(std::wstring_view styleName) const;
	double _GetIconSize() const;

	void _SetErrorState();
	void _SetEnabledState();

	int _key = 0;
	winrt::Magpie::VisualType _visualType = winrt::Magpie::VisualType::Small;
	bool _isError = false;

	Controls::ContentPresenter _keyPresenter{ nullptr };
	IsEnabledChanged_revoker _isEnabledChangedRevoker;
};

}

namespace winrt::Magpie::factory_implementation {

struct KeyVisual : KeyVisualT<KeyVisual, implementation::KeyVisual> {
};

}
