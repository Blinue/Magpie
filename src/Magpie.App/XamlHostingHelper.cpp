#include "pch.h"
#include "XamlHostingHelper.h"
#include <CoreWindow.h>
#include "Win32Utils.h"

using namespace winrt;
using namespace Windows::UI::Xaml::Hosting;

namespace winrt::Magpie::App {

XamlHostingHelper::ManagerWrapper::ManagerWrapper() {
	_windowsXamlManager = WindowsXamlManager::InitializeForCurrentThread();

	if (!Win32Utils::GetOSVersion().IsWin11()) {
		// Win10 中隐藏 DesktopWindowXamlSource 窗口
		if (CoreWindow coreWindow = CoreWindow::GetForCurrentThread()) {
			HWND hwndDWXS;
			coreWindow.as<ICoreWindowInterop>()->get_WindowHandle(&hwndDWXS);
			ShowWindow(hwndDWXS, SW_HIDE);
		}
	}
}

XamlHostingHelper::ManagerWrapper::~ManagerWrapper() {
	if (!_windowsXamlManager) {
		return;
	}

	_windowsXamlManager.Close();

	// 做最后的清理，见
	// https://learn.microsoft.com/en-us/uwp/api/windows.ui.xaml.hosting.windowsxamlmanager.close
	MSG msg;
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
		DispatchMessage(&msg);
	}
}

}
