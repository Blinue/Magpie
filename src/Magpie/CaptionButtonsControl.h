#pragma once
#include "CaptionButtonsControl.g.h"

namespace winrt::Magpie::implementation {

enum class CaptionButton {
	Minimize = HTMINBUTTON,
	Maximize = HTMAXBUTTON,
	Close = HTCLOSE
};

struct CaptionButtonsControl : CaptionButtonsControlT<CaptionButtonsControl> {
	Size CaptionButtonSize() const;

	void HoverButton(CaptionButton button);

	void PressButton(CaptionButton button);

	void ReleaseButton(CaptionButton button);

	void ReleaseButtons();

	void LeaveButtons();

	void IsWindowMaximized(bool value);
	void IsWindowActive(bool value);

private:
	std::optional<CaptionButton> _pressedButton;
	// 用于避免重复设置状态
	bool _allInNormal = true;
	bool _isWindowMaximized = false;
	bool _isWindowActive = true;
};

}

namespace winrt::Magpie::factory_implementation {

struct CaptionButtonsControl : CaptionButtonsControlT<CaptionButtonsControl, implementation::CaptionButtonsControl> {
};

}
