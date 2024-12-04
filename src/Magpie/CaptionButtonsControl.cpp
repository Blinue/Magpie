#include "pch.h"
#include "CaptionButtonsControl.h"
#if __has_include("CaptionButtonsControl.g.cpp")
#include "CaptionButtonsControl.g.cpp"
#endif

namespace winrt::Magpie::implementation {

Size CaptionButtonsControl::CaptionButtonSize() const {
	ResourceDictionary resources = Resources();
	return {
		(float)unbox_value<double>(resources.Lookup(box_value(L"CaptionButtonWidth"))),
		(float)unbox_value<double>(resources.Lookup(box_value(L"CaptionButtonHeight")))
	};
}

// 鼠标移动到某个按钮上时调用
void CaptionButtonsControl::HoverButton(CaptionButton button) {
	if (_pressedButton) {
		bool hoveringOnPressedButton = _pressedButton.value() == button;
		_allInNormal = !hoveringOnPressedButton;

		VisualStateManager::GoToState(MinimizeButton(),
			hoveringOnPressedButton && button == CaptionButton::Minimize ? L"Pressed" : L"Normal", false);
		VisualStateManager::GoToState(MaximizeButton(),
			hoveringOnPressedButton && button == CaptionButton::Maximize ? L"Pressed" : L"Normal", false);
		VisualStateManager::GoToState(CloseButton(),
			hoveringOnPressedButton && button == CaptionButton::Close ? L"Pressed" : L"Normal", false);
	} else {
		_allInNormal = false;

		const wchar_t* activeState = _isWindowActive ? L"Normal" : L"NotActive";
		VisualStateManager::GoToState(MinimizeButton(),
			button == CaptionButton::Minimize ? L"PointerOver" : activeState, false);
		VisualStateManager::GoToState(MaximizeButton(),
			button == CaptionButton::Maximize ? L"PointerOver" : activeState, false);
			VisualStateManager::GoToState(CloseButton(),
				button == CaptionButton::Close ? L"PointerOver" : activeState, false);
	}
}

// 在某个按钮上按下鼠标时调用
void CaptionButtonsControl::PressButton(CaptionButton button) {
	_allInNormal = false;
	_pressedButton = button;

	VisualStateManager::GoToState(MinimizeButton(),
		button == CaptionButton::Minimize ? L"Pressed" : L"Normal", false);
	VisualStateManager::GoToState(MaximizeButton(),
		button == CaptionButton::Maximize ? L"Pressed" : L"Normal", false);
	VisualStateManager::GoToState(CloseButton(),
		button == CaptionButton::Close ? L"Pressed" : L"Normal", false);
}

// 在标题栏按钮上释放鼠标时调用
void CaptionButtonsControl::ReleaseButton(CaptionButton button) {
	// 在某个按钮上按下然后释放视为点击，即使中途离开过也是如此，因为 HoverButton 和
	// LeaveButtons 都不改变 _pressedButton
	const bool clicked = _pressedButton && _pressedButton.value() == button;

	if (clicked) {
		// 用户点击了某个按钮
		HWND hwndMain = (HWND)Application::Current().as<App>().HwndMain();

		switch (_pressedButton.value()) {
		case CaptionButton::Minimize:
		{
			PostMessage(hwndMain, WM_SYSCOMMAND, SC_MINIMIZE, 0);
			break;
		}
		case CaptionButton::Maximize:
		{
			POINT cursorPos;
			GetCursorPos(&cursorPos);

			PostMessage(
				hwndMain,
				WM_SYSCOMMAND,
				_isWindowMaximized ? SC_RESTORE : SC_MAXIMIZE,
				MAKELPARAM(cursorPos.x, cursorPos.y)
			);
			break;
		}
		case CaptionButton::Close:
		{
			PostMessage(hwndMain, WM_SYSCOMMAND, SC_CLOSE, 0);
			break;
		}
		}
	}

	_pressedButton.reset();

	// 如果点击了某个按钮则清空状态，因为此时窗口状态已改变。如果在某个按钮上按下然后在
	// 其他按钮上释放，不视为点击，则将当前鼠标所在的按钮状态置为 PointerOver
	_allInNormal = clicked;
	VisualStateManager::GoToState(MinimizeButton(),
		!clicked && button == CaptionButton::Minimize ? L"PointerOver" : L"Normal", false);
	VisualStateManager::GoToState(MaximizeButton(),
		!clicked && button == CaptionButton::Maximize ? L"PointerOver" : L"Normal", false);
	VisualStateManager::GoToState(CloseButton(),
		!clicked && button == CaptionButton::Close ? L"PointerOver" : L"Normal", false);
}

// 在非标题按钮上释放鼠标时调用
void CaptionButtonsControl::ReleaseButtons() {
	if (!_pressedButton) {
		return;
	}
	_pressedButton.reset();

	LeaveButtons();
}

// 离开标题按钮时调用，不更改 _pressedButton
void CaptionButtonsControl::LeaveButtons() {
	if (_allInNormal) {
		return;
	}
	_allInNormal = true;

	const wchar_t* activeState = _isWindowActive ? L"Normal" : L"NotActive";
	VisualStateManager::GoToState(MinimizeButton(), activeState, true);
	VisualStateManager::GoToState(MaximizeButton(), activeState, true);
	VisualStateManager::GoToState(CloseButton(), activeState, true);
}

void CaptionButtonsControl::IsWindowMaximized(bool value) {
	if (_isWindowMaximized == value) {
		return;
	}

	if (VisualStateManager::GoToState(MaximizeButton(),
		value ? L"WindowStateMaximized" : L"WindowStateNormal", false)) {
		_isWindowMaximized = value;
	}
}

void CaptionButtonsControl::IsWindowActive(bool value) {
	_isWindowActive = value;

	const wchar_t* activeState = value ? L"Normal" : L"NotActive";
	VisualStateManager::GoToState(MinimizeButton(), activeState, false);
	VisualStateManager::GoToState(MaximizeButton(), activeState, false);
	VisualStateManager::GoToState(CloseButton(), activeState, false);
}

}
