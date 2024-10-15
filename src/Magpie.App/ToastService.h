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

private:
	ToastService() = default;

	void _ToastThreadProc() noexcept;

	std::thread _toastThread;
};

}
