#pragma once
#include "ToastPage.h"

namespace Magpie {

class ToastService {
public:
	static ToastService& Get() noexcept {
		static ToastService instance;
		return instance;
	}

	ToastService(const ToastService&) = delete;
	ToastService(ToastService&&) = delete;

	void Initialize() noexcept;

	void Uninitialize() noexcept;

	void ShowMessageOnWindow(std::wstring_view title, std::wstring_view message, HWND hwndTarget) const noexcept;

	void ShowMessageInApp(std::wstring_view title, std::wstring_view message) const noexcept;

private:
	ToastService() = default;

	void _ToastThreadProc() noexcept;

	static LRESULT CALLBACK _ToastWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	// 确保 _dispatcher 完成初始化
	const winrt::CoreDispatcher& _Dispatcher() const noexcept;

	std::thread _toastThread;

	winrt::CoreDispatcher _dispatcher{ nullptr };
	std::atomic<bool> _dispatcherInitialized = false;

	// 只能在 toast 线程访问
	winrt::com_ptr<winrt::Magpie::implementation::ToastPage> _toastPage{ nullptr };
	HWND _hwndToast = NULL;
};

}
