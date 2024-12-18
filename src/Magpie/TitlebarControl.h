#pragma once
#include "TitleBarControl.g.h"
#include "CaptionButtonsControl.h"

namespace winrt::Magpie::implementation {
struct TitleBarControl : TitleBarControlT<TitleBarControl>,
                         wil::notify_property_changed_base<TitleBarControl> {
	TitleBarControl();

	void Loading(FrameworkElement const&, IInspectable const&);

	Imaging::SoftwareBitmapSource Logo() const noexcept {
		return _logo;
	}

	void IsWindowActive(bool value);

	CaptionButtonsControl& CaptionButtons() {
		return *get_self<CaptionButtonsControl>(TitleBarControlT::CaptionButtons());
	}

private:
	Imaging::SoftwareBitmapSource _logo{ nullptr };
};
}

namespace winrt::Magpie::factory_implementation {

struct TitleBarControl : TitleBarControlT<TitleBarControl, implementation::TitleBarControl> {
};

}
