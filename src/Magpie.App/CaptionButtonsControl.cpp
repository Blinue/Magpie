#include "pch.h"
#include "CaptionButtonsControl.h"
#if __has_include("CaptionButtonsControl.g.cpp")
#include "CaptionButtonsControl.g.cpp"
#endif

namespace winrt::Magpie::App::implementation {

double CaptionButtonsControl::CaptionButtonWidth() const noexcept {
	return unbox_value<double>(Resources().Lookup(box_value(L"CaptionButtonWidth")));
}

void CaptionButtonsControl::HoverButton(CaptionButton button) {
	_allButtonsReleased = false;

	if (_lastPressedButton) {
		bool hoveringOnPressedButton = _lastPressedButton.value() == button;

		VisualStateManager::GoToState(MinimizeButton(),
			hoveringOnPressedButton && button == CaptionButton::Minimize ? L"Pressed" : L"Normal", false);
		VisualStateManager::GoToState(MaximizeButton(),
			hoveringOnPressedButton && button == CaptionButton::Maximize ? L"Pressed" : L"Normal", false);
		VisualStateManager::GoToState(CloseButton(),
			hoveringOnPressedButton && button == CaptionButton::Close ? L"Pressed" : L"Normal", false);
	} else {
		VisualStateManager::GoToState(MinimizeButton(),
			button == CaptionButton::Minimize ? L"PointerOver" : L"Normal", false);
		VisualStateManager::GoToState(MaximizeButton(),
			button == CaptionButton::Maximize ? L"PointerOver" : L"Normal", false);
		VisualStateManager::GoToState(CloseButton(),
			button == CaptionButton::Close ? L"PointerOver" : L"Normal", false);
	}
}

void CaptionButtonsControl::PressButton(CaptionButton button) {
	_allButtonsReleased = false;
	_lastPressedButton = button;

	VisualStateManager::GoToState(MinimizeButton(),
		button == CaptionButton::Minimize ? L"Pressed" : L"Normal", false);
	VisualStateManager::GoToState(MaximizeButton(),
		button == CaptionButton::Maximize ? L"Pressed" : L"Normal", false);
	VisualStateManager::GoToState(CloseButton(),
		button == CaptionButton::Close ? L"Pressed" : L"Normal", false);
}

void CaptionButtonsControl::ReleaseButton(CaptionButton button) {
	bool clicked = _lastPressedButton && _lastPressedButton.value() == button;
	
	if (clicked) {
		HWND hwndMain = (HWND)Application::Current().as<App>().HwndMain();

		switch (_lastPressedButton.value()) {
		case CaptionButton::Minimize:
			PostMessage(hwndMain, WM_SYSCOMMAND, SC_MINIMIZE | HTMINBUTTON, 0);
			break;
		case CaptionButton::Maximize:
		{
			POINT cursorPos;
			GetCursorPos(&cursorPos);

			PostMessage(
				hwndMain,
				WM_SYSCOMMAND,
				(_isWindowMaximized ? SC_RESTORE : SC_MAXIMIZE) | HTMAXBUTTON,
				MAKELPARAM(cursorPos.x, cursorPos.y)
			);
			break;
		}
		case CaptionButton::Close:
			PostMessage(hwndMain, WM_SYSCOMMAND, SC_CLOSE, 0);
			break;
		}
	}
	
	_lastPressedButton.reset();

	VisualStateManager::GoToState(MinimizeButton(), L"Normal", false);
	VisualStateManager::GoToState(MaximizeButton(), L"Normal", false);
	VisualStateManager::GoToState(CloseButton(), L"Normal", false);
}

void CaptionButtonsControl::ReleaseButtons() {
	if (!_lastPressedButton) {
		return;
	}
	_lastPressedButton.reset();

	LeaveButtons();
}

void CaptionButtonsControl::LeaveButtons() {
	VisualStateManager::GoToState(MinimizeButton(), L"Normal", true);
	VisualStateManager::GoToState(MaximizeButton(), L"Normal", true);
	VisualStateManager::GoToState(CloseButton(), L"Normal", true);
}

void CaptionButtonsControl::IsWindowMaximized(bool value) {
	_isWindowMaximized = value;

	VisualStateManager::GoToState(MaximizeButton(),
		value ? L"WindowStateMaximized" : L"WindowStateNormal", false);
}

}
