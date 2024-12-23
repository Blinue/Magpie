#pragma once
#include "XamlWindow.h"
#include <winrt/Magpie.h>
#include "RootPage.h"

namespace Magpie {

class MainWindow : public XamlWindowT<MainWindow, winrt::com_ptr<winrt::Magpie::implementation::RootPage>> {
	using base_type = XamlWindowT<MainWindow, winrt::com_ptr<winrt::Magpie::implementation::RootPage>>;
	friend WindowBaseT<MainWindow>;
public:
	bool Create() noexcept;

	void Show() const noexcept;

protected:
	LRESULT _MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

private:
	std::pair<POINT, SIZE> _CreateWindow() noexcept;

	void _UpdateTheme() noexcept;

	static LRESULT CALLBACK _TitleBarWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

	LRESULT _TitleBarMessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

	void _ResizeTitleBarWindow() noexcept;

	MultithreadEvent<bool>::EventRevoker _appThemeChangedRevoker;

	HWND _hwndTitleBar = NULL;
	HWND _hwndMaximizeButton = NULL;
	bool _trackingMouse = false;
};

}
