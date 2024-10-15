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

	void ShowMessage(std::wstring_view message) noexcept;

private:
	ToastService() = default;

	void _ToastThreadProc() noexcept;

	// 确保 _dispatcher 完成初始化
	const CoreDispatcher& _Dispatcher() noexcept;

	std::thread _toastThread;

	ToastPage _toastPage{ nullptr };
	CoreDispatcher _dispatcher{ nullptr };
	std::atomic<bool> _dispatcherInitialized = false;
	// 只能在主线程访问，省下检查 _dispatcherInitialized 的开销
	bool _dispatcherInitializedCache = false;
};

}
