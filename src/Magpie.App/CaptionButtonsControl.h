#pragma once
#include "CaptionButtonsControl.g.h"

namespace winrt::Magpie::App::implementation {

struct CaptionButtonsControl : CaptionButtonsControlT<CaptionButtonsControl> {
	CaptionButtonsControl() {}

	double CaptionButtonWidth() const noexcept;

	void CloseButton_Click(IInspectable const&, RoutedEventArgs const&) noexcept;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct CaptionButtonsControl : CaptionButtonsControlT<CaptionButtonsControl, implementation::CaptionButtonsControl> {
};

}
