#include "pch.h"
#include <dispatcherqueue.h>
#include "MagApp.h"
#include "MagRuntime.h"


namespace Magpie::Core {

class MagRuntime::Impl {
public:
	Impl() = default;
	Impl(const Impl&) = delete;
	Impl(Impl&&) = default;

	~Impl() {
		Stop();

		if (_magWindThread.joinable()) {
			_magWindThread.join();
		}
	}

	void Run(HWND hwndSrc, const MagOptions& options) {
		if (_running) {
			return;
		}

		_hwndSrc = hwndSrc;
		_running = true;
		_isRunningChangedEvent(true);

		if (_magWindThread.joinable()) {
			_magWindThread.join();
		}

		_magWindThread = std::thread([=, this, options(options)]() mutable {
			winrt::init_apartment(winrt::apartment_type::multi_threaded);

			DispatcherQueueOptions dqOptions{};
			dqOptions.dwSize = sizeof(DispatcherQueueOptions);
			dqOptions.threadType = DQTYPE_THREAD_CURRENT;

			HRESULT hr = CreateDispatcherQueueController(
				dqOptions,
				(ABI::Windows::System::IDispatcherQueueController**)winrt::put_abi(_dqc)
			);
			if (FAILED(hr)) {
				_running = false;
				_isRunningChangedEvent(false);
				return;
			}

			MagApp& app = MagApp::Get();
			app.Run((HWND)hwndSrc, std::move(options), _dqc.DispatcherQueue());

			_running = false;
			_dqc = nullptr;
			_isRunningChangedEvent(false);
		});
	}

	void ToggleOverlay() {
		if (!_running || !_dqc) {
			return;
		}

		_dqc.DispatcherQueue().TryEnqueue([]() {
			MagApp::Get().ToggleOverlay();
		});
	}

	void Stop() {
		if (!_running || !_dqc) {
			return;
		}

		_dqc.DispatcherQueue().TryEnqueue([]() {
			MagApp::Get().Stop();
		});

		if (_magWindThread.joinable()) {
			_magWindThread.join();
		}
	}

	bool IsRunning() const {
		return _running;
	}

	HWND HwndSrc() const {
		return _running ? _hwndSrc : 0;
	}

	// 调用者应处理线程同步
	winrt::event_token IsRunningChanged(winrt::delegate<bool> const& handler) {
		return _isRunningChangedEvent.add(handler);
	}

	void IsRunningChanged(winrt::event_token const& token) noexcept {
		_isRunningChangedEvent.remove(token);
	}

private:
	std::thread _magWindThread;
	std::atomic<bool> _running = false;
	HWND _hwndSrc = 0;
	winrt::DispatcherQueueController _dqc{ nullptr };

	winrt::event<winrt::delegate<bool>> _isRunningChangedEvent;
};

MagRuntime::MagRuntime() : _impl(new Impl()) {}

MagRuntime::~MagRuntime() {
	delete _impl;
}

HWND MagRuntime::HwndSrc() const {
	return _impl->HwndSrc();
}

void MagRuntime::Run(HWND hwndSrc, const MagOptions& options) {
	return _impl->Run(hwndSrc, options);
}

void MagRuntime::ToggleOverlay() {
	return _impl->ToggleOverlay();
}

void MagRuntime::Stop() {
	return _impl->Stop();
}

bool MagRuntime::IsRunning() const {
	return _impl->IsRunning();
}

winrt::event_token MagRuntime::IsRunningChanged(winrt::delegate<bool> const& handler) {
	return _impl->IsRunningChanged(handler);
}

void MagRuntime::IsRunningChanged(winrt::event_token const& token) noexcept {
	_impl->IsRunningChanged(token);
}

}
