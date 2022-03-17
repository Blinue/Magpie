#pragma once
#include "pch.h"
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>


class XamlHost {
public:
	XamlHost() = default;

	XamlHost(const XamlHost&) = delete;
	XamlHost(XamlHost&&) = delete;

	~XamlHost() {
		Detach();
	}

	bool HasAttach() const {
		return !!_xamlSource;
	}

	void Attach(HWND parent, winrt::Windows::UI::Xaml::UIElement xamlElement);

	void Detach();

	bool PreHandleMessage(const MSG& msg);

	void HandleMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	void _OnResize();

	winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource _xamlSource{ nullptr };
	winrt::com_ptr<IDesktopWindowXamlSourceNative2> _xamlSourceNative2;

	HWND _hwndXamlIsland = NULL;
	HWND _hwndParent = NULL;
};
