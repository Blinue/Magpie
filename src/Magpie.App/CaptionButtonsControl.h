#pragma once
#include "CaptionButtonsControl.g.h"

namespace winrt::Magpie::App::implementation {

struct CaptionButtonsControl : CaptionButtonsControlT<CaptionButtonsControl> {
	CaptionButtonsControl() {}

	double CaptionButtonWidth() const noexcept;

	void HoverButton(CaptionButton button);

	void PressButton(CaptionButton button);

	void ReleaseButton(CaptionButton button);

	void ReleaseButtons();

	void LeaveButtons();

	void IsWindowMaximized(bool value);

private:
	std::optional<CaptionButton> _lastPressedButton;
	bool _allButtonsReleased = true;
	bool _isWindowMaximized = false;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct CaptionButtonsControl : CaptionButtonsControlT<CaptionButtonsControl, implementation::CaptionButtonsControl> {
};

}
