#include "pch.h"
#include "KirikiriWindow.h"
#include "HungWindow.h"
#include "TopmostWindow.h"
#include "PopupHostWindow.h"
#include "HideCursorWindow.h"

int APIENTRY wWinMain(
	_In_ HINSTANCE /*hInstance*/,
	_In_opt_ HINSTANCE /*hPrevInstance*/,
	_In_ LPWSTR /*lpCmdLine*/,
	_In_ int /*nCmdShow*/
) {
	// 模拟 TVP(KIRIKIRI) 2 引擎窗口
	KirikiriWindow window;
	
	// 模拟挂起的窗口
	// HungWindow window;
	
	// 模拟中途置顶/取消置顶的窗口
	// TopmostWindow window;

	// 模拟有弹窗的窗口
	// PopupHostWindow window;

	// 模拟隐藏光标的窗口
	// HideCursorWindow window;

	if (!window.Create()) {
		return false;
	}

	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}
