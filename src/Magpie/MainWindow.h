#pragma once
#include "XamlWindow.h"
#include <winrt/Magpie.h>

namespace Magpie {

class MainWindow : public XamlWindowT<MainWindow, winrt::Magpie::RootPage> {
	friend base_type;
public:
	bool Create(HINSTANCE hInstance, winrt::Point windowCenter, winrt::Size windowSizeInDips, bool isMaximized) noexcept;

	void Show() const noexcept;

protected:
	LRESULT _MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

private:
	std::pair<POINT, SIZE> _CreateWindow(HINSTANCE hInstance, winrt::Point windowCenter, winrt::Size windowSizeInDips) noexcept;

	void _UpdateTheme() noexcept;

	static LRESULT CALLBACK _TitleBarWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

	LRESULT _TitleBarMessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

	void _ResizeTitleBarWindow() noexcept;

	HWND _hwndTitleBar = NULL;
	HWND _hwndMaximizeButton = NULL;
	bool _trackingMouse = false;
};

}
