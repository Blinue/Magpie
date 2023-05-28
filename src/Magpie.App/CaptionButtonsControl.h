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
	std::optional<CaptionButton> _pressedButton;
	// 用于避免重复设置状态
	bool _allInNormal = true;
	bool _isWindowMaximized = false;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct CaptionButtonsControl : CaptionButtonsControlT<CaptionButtonsControl, implementation::CaptionButtonsControl> {
};

}
