#pragma once

class WindowAnimationDisabler {
public:
	WindowAnimationDisabler(HWND hWnd);
	~WindowAnimationDisabler();

private:
	HWND _hWnd;
};
