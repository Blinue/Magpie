#include "pch.h"
#include "XamlHost.h"
#include <winrt/Windows.UI.Core.h>
#include <CoreWindow.h>


namespace winrt {
using namespace Windows::UI::Xaml::Hosting;
}


void XamlHost::Attach(HWND parent, winrt::Windows::UI::Xaml::UIElement xamlElement) {
	if (HasAttach()) {
		Detach();
	}
	_xamlSource = winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource();
	_xamlSourceNative2 = _xamlSource.as<IDesktopWindowXamlSourceNative2>();

	auto interop = _xamlSource.as<IDesktopWindowXamlSourceNative>();
	interop->AttachToWindow(parent);
	_hwndParent = parent;

	interop->get_WindowHandle(&_hwndXamlIsland);
	_xamlSource.Content(xamlElement);

	_OnResize();

	// 防止关闭时出现 DesktopWindowXamlSource 窗口
	auto coreWindow = winrt::Windows::UI::Core::CoreWindow::GetForCurrentThread();
	if (coreWindow) {
		HWND hwndDWXS;
		coreWindow.as<ICoreWindowInterop>()->get_WindowHandle(&hwndDWXS);
		ShowWindow(hwndDWXS, SW_HIDE);
	}
}

void XamlHost::Detach() {
	if (!HasAttach()) {
		return;
	}

	_xamlSource.Close();
	_xamlSource = nullptr;
}

bool XamlHost::PreHandleMessage(const MSG& msg) {
	if (!HasAttach()) {
		return false;
	}

	BOOL processed = FALSE;
	HRESULT hr = _xamlSourceNative2->PreTranslateMessage(&msg, &processed);
	if (FAILED(hr)) {
		processed = FALSE;
	}

	if (processed) {
		return true;
	}

	return false;
}

void XamlHost::HandleMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	if (!HasAttach()) {
		return;
	}

	switch (message) {
	case WM_SIZE:
		_OnResize();
		break;
	case WM_SETFOCUS:
		if (_hwndXamlIsland) {
			SetFocus(_hwndXamlIsland);
		}
		break;
	case WM_DESTROY:
		Detach();
		break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;
	}
	default:
		break;
	}
}

void XamlHost::_OnResize() {
	if (!HasAttach()) {
		return;
	}

	RECT clientRect;
	GetClientRect(_hwndParent, &clientRect);
	SetWindowPos(_hwndXamlIsland, NULL, 0, 0, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, SWP_SHOWWINDOW);
}
