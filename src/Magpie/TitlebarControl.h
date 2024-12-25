#pragma once
#include "TitleBarControl.g.h"

namespace winrt::Magpie::implementation {

struct CaptionButtonsControl;

struct TitleBarControl : TitleBarControlT<TitleBarControl>,
                         wil::notify_property_changed_base<TitleBarControl> {
	TitleBarControl();

	void TitleBarControl_Loading(FrameworkElement const&, IInspectable const&);

	Imaging::SoftwareBitmapSource Logo() const noexcept {
		return _logo;
	}

	void IsWindowActive(bool value);

	CaptionButtonsControl& CaptionButtons() noexcept;

private:
	Imaging::SoftwareBitmapSource _logo{ nullptr };
};

}

namespace winrt::Magpie::factory_implementation {

struct TitleBarControl : TitleBarControlT<TitleBarControl, implementation::TitleBarControl> {
};

}
