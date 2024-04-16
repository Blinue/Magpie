#pragma once
#include "TitleBarControl.g.h"

namespace winrt::Magpie::App::implementation {
struct TitleBarControl : TitleBarControlT<TitleBarControl>,
                         wil::notify_property_changed_base<TitleBarControl> {
	TitleBarControl();

	void Loading(FrameworkElement const&, IInspectable const&);

	Imaging::SoftwareBitmapSource Logo() const noexcept {
		return _logo;
	}

	void IsWindowActive(bool value);

private:
	Imaging::SoftwareBitmapSource _logo{ nullptr };
};
}

namespace winrt::Magpie::App::factory_implementation {

struct TitleBarControl : TitleBarControlT<TitleBarControl, implementation::TitleBarControl> {
};

}
