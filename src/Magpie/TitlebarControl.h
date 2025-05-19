#pragma once
#include "TitleBarControl.g.h"
#include "Event.h"

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

	Point LeftBottomPoint() noexcept;

	::Magpie::Event<> LeftBottomPointChanged;

private:
	Imaging::SoftwareBitmapSource _logo{ nullptr };
};

}

BASIC_FACTORY(TitleBarControl)
