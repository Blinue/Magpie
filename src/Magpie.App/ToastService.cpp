#include "pch.h"
#include "ToastService.h"
#include "XamlHostingHelper.h"
#include "CommonSharedConstants.h"
#include "Utils.h"
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>

using namespace winrt;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Hosting;

namespace winrt::Magpie::App {

void ToastService::Initialize() noexcept {
	_toastThread = std::thread(std::bind_front(&ToastService::_ToastThreadProc, this));
}

void ToastService::Uninitialize() noexcept {
	if (!_toastThread.joinable()) {
		return;
	}

	const HANDLE hToastThread = _toastThread.native_handle();

	if (!wil::handle_wait(hToastThread, 0)) {
		const DWORD threadId = GetThreadId(hToastThread);

		// 持续尝试直到 _toastThread 创建了消息队列
		while (!PostThreadMessage(threadId, CommonSharedConstants::WM_TOAST_QUIT, 0, 0)) {
			if (wil::handle_wait(hToastThread, 1)) {
				break;
			}
		}
	}

	_toastThread.join();
}

void ToastService::_ToastThreadProc() noexcept {
#ifdef _DEBUG
	SetThreadDescription(GetCurrentThread(), L"Toast 线程");
#endif

	winrt::init_apartment(winrt::apartment_type::single_threaded);

	static Utils::Ignore _ = [] {
		WNDCLASSEXW wcex{
			.cbSize = sizeof(wcex),
			.lpfnWndProc = DefWindowProc,
			.hInstance = wil::GetModuleInstanceHandle(),
			.lpszClassName = CommonSharedConstants::TOAST_WINDOW_CLASS_NAME
		};
		RegisterClassEx(&wcex);

		return Utils::Ignore();
	}();

	// 创建窗口失败也应进入消息循环。Win10 中关闭任意线程的 DesktopWindowXamlSource 都会使主线程会崩溃，
	// 在程序退出前，xamlSource 不能析构。见 https://github.com/microsoft/terminal/pull/15397
	HWND hwndToast = CreateWindowEx(
		WS_EX_NOREDIRECTIONBITMAP | WS_EX_TOOLWINDOW,
		CommonSharedConstants::TOAST_WINDOW_CLASS_NAME,
		L"Toast",
		WS_POPUP | WS_VISIBLE,
		200, 200, 0, 0,
		NULL,
		NULL,
		wil::GetModuleInstanceHandle(),
		nullptr
	);

	// DesktopWindowXamlSource 在控件之前创建则无需调用 WindowsXamlManager::InitializeForCurrentThread
	DesktopWindowXamlSource xamlSource;
	com_ptr<IDesktopWindowXamlSourceNative2> xamlSourceNative2 =
		xamlSource.try_as<IDesktopWindowXamlSourceNative2>();
	
	xamlSourceNative2->AttachToWindow(hwndToast);

	HWND hwndXamlIsland;
	xamlSourceNative2->get_WindowHandle(&hwndXamlIsland);
	SetWindowPos(hwndXamlIsland, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW);

	ToastPage toastPage;
	xamlSource.Content(toastPage);

	auto tt = toastPage.FindName(L"MessageTeachingTip").as<MUXC::TeachingTip>();
	tt.IsOpen(true);

	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0)) {
		if (msg.message == CommonSharedConstants::WM_TOAST_QUIT) {
			DestroyWindow(hwndToast);
			break;
		}

		{
			BOOL processed = FALSE;
			HRESULT hr = xamlSourceNative2->PreTranslateMessage(&msg, &processed);
			if (SUCCEEDED(hr) && processed) {
				continue;
			}
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// 必须手动重置 Content，否则会内存泄露
	xamlSource.Content(nullptr);
	xamlSource.Close();
}

}
