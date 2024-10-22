#include "pch.h"
#include "ToastService.h"
#include "CommonSharedConstants.h"
#include "Utils.h"
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include "XamlUtils.h"

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

void ToastService::ShowMessageOnWindow(std::wstring_view message, HWND hwndTarget) const noexcept {
	_Dispatcher().RunAsync(CoreDispatcherPriority::Normal, [this, message(std::wstring(message)), hwndTarget]() {
		_toastPage.ShowMessageOnWindow(message, (uint64_t)hwndTarget);
	});
}

void ToastService::ShowMessageInApp(std::wstring_view message) const noexcept {
	_Dispatcher().RunAsync(CoreDispatcherPriority::Normal, [this, message(std::wstring(message))]() {
		_toastPage.ShowMessageInApp(message);
	});
}

void ToastService::_ToastThreadProc() noexcept {
#ifdef _DEBUG
	SetThreadDescription(GetCurrentThread(), L"Toast 线程");
#endif

	winrt::init_apartment(winrt::apartment_type::single_threaded);

	static Utils::Ignore _ = [] {
		WNDCLASSEXW wcex{
			.cbSize = sizeof(wcex),
			.lpfnWndProc = _ToastWndProc,
			.hInstance = wil::GetModuleInstanceHandle(),
			.lpszClassName = CommonSharedConstants::TOAST_WINDOW_CLASS_NAME
		};
		RegisterClassEx(&wcex);

		return Utils::Ignore();
	}();

	// 创建窗口失败也应进入消息循环。Win10 中关闭任意线程的 DesktopWindowXamlSource 都会使主线程会崩溃，
	// 在程序退出前，xamlSource 不能析构。见 https://github.com/microsoft/terminal/pull/15397
	_hwndToast = CreateWindowEx(
		WS_EX_NOREDIRECTIONBITMAP | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT,
		CommonSharedConstants::TOAST_WINDOW_CLASS_NAME,
		L"Toast",
		WS_POPUP,
		0, 0, 0, 0,
		NULL,
		NULL,
		wil::GetModuleInstanceHandle(),
		nullptr
	);
	SetWindowPos(_hwndToast, NULL, 0, 0, 0, 0,
		SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOSIZE | SWP_NOREDRAW | SWP_NOZORDER);

	// DesktopWindowXamlSource 在控件之前创建则无需调用 WindowsXamlManager::InitializeForCurrentThread
	DesktopWindowXamlSource xamlSource;
	com_ptr<IDesktopWindowXamlSourceNative2> xamlSourceNative2 =
		xamlSource.try_as<IDesktopWindowXamlSourceNative2>();
	
	xamlSourceNative2->AttachToWindow(_hwndToast);

	HWND hwndXamlIsland;
	xamlSourceNative2->get_WindowHandle(&hwndXamlIsland);
	SetWindowPos(hwndXamlIsland, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW);

	_toastPage = ToastPage((uint64_t)_hwndToast);
	xamlSource.Content(_toastPage);

	_dispatcher = _toastPage.Dispatcher();
	// 如果主线程正在等待则唤醒主线程
	_dispatcherInitialized.store(true, std::memory_order_release);
	_dispatcherInitialized.notify_one();

	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0)) {
		if (msg.message == CommonSharedConstants::WM_TOAST_QUIT) {
			DestroyWindow(_hwndToast);
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

LRESULT ToastService::_ToastWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_MOVE) {
		if (Get()._toastPage) {
			// 使弹窗随窗口移动
			XamlUtils::RepositionXamlPopups(Get()._toastPage.XamlRoot(), false);
		}
		
		return 0;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

const CoreDispatcher& ToastService::_Dispatcher() const noexcept {
	_dispatcherInitialized.wait(false, std::memory_order_acquire);
	return _dispatcher;
}

}
