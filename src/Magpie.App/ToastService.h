#pragma once
#include <winrt/Magpie.App.h>

namespace winrt::Magpie::App {

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

	void ShowMessageOnWindow(std::wstring_view message, HWND hWnd) noexcept;

private:
	ToastService() = default;

	void _ToastThreadProc() noexcept;

	static LRESULT CALLBACK _ToastWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	// 确保 _dispatcher 完成初始化。供主线程使用，toast 线程应直接使用 _dispatcher
	const CoreDispatcher& _Dispatcher() noexcept;

	std::thread _toastThread;

	CoreDispatcher _dispatcher{ nullptr };
	std::atomic<bool> _dispatcherInitialized = false;
	// 只能在主线程访问，省下检查 _dispatcherInitialized 的开销
	bool _dispatcherInitializedCache = false;

	// 只能在 toast 线程访问
	ToastPage _toastPage{ nullptr };
	HWND _hwndToast = NULL;
};

}
