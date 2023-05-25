#pragma once
#include "XamlWindow.h"
#include <winrt/Magpie.App.h>

namespace Magpie {

class MainWindow : public XamlWindowT<MainWindow, winrt::Magpie::App::MainPage> {
	friend class base_type;
public:
	bool Create(HINSTANCE hInstance, const RECT& windowRect, bool isMaximized) noexcept;

	void Show() const noexcept;

protected:
	LRESULT _MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

private:
	void _UpdateTheme();

	static LRESULT CALLBACK _TitleBarWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

	LRESULT _TitleBarMessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

	void _ResizeTitleBarWindow() noexcept;

	HWND _hwndTitleBar = NULL;
};

}
