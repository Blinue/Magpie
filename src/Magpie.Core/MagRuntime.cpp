#include "pch.h"
#include <dispatcherqueue.h>
#include "MagApp.h"
#include "MagRuntime.h"
#include "Logger.h"


namespace Magpie::Core {

MagRuntime::MagRuntime() : _magWindThread(std::bind(&MagRuntime::_MagWindThreadProc, this)) {
}

MagRuntime::~MagRuntime() {
	Stop();

	if (_magWindThread.joinable()) {
		DWORD magWndThreadId = GetThreadId(_magWindThread.native_handle());
		PostThreadMessage(magWndThreadId, WM_QUIT, 0, 0);
		_magWindThread.join();
	}
}

void MagRuntime::Run(HWND hwndSrc, const MagOptions& options) {
	if (_running) {
		return;
	}

	_hwndSrc = hwndSrc;
	_running = true;
	_isRunningChangedEvent(true);

	_EnsureDispatcherQueue();
	_dqc.DispatcherQueue().TryEnqueue([this, hwndSrc, options(options)]() mutable {
		MagApp::Get().Start(hwndSrc, std::move(options));
	});
}

void MagRuntime::ToggleOverlay() {
	if (!_running) {
		return;
	}

	_EnsureDispatcherQueue();
	_dqc.DispatcherQueue().TryEnqueue([]() {
		MagApp::Get().ToggleOverlay();
	});
}

void MagRuntime::Stop() {
	if (!_running) {
		return;
	}

	_EnsureDispatcherQueue();
	_dqc.DispatcherQueue().TryEnqueue([]() {
		MagApp::Get().Stop();
	});
}

void MagRuntime::_MagWindThreadProc() noexcept {
	winrt::init_apartment(winrt::apartment_type::single_threaded);

	DispatcherQueueOptions dqOptions{};
	dqOptions.dwSize = sizeof(DispatcherQueueOptions);
	dqOptions.threadType = DQTYPE_THREAD_CURRENT;

	HRESULT hr = CreateDispatcherQueueController(
		dqOptions,
		(PDISPATCHERQUEUECONTROLLER*)winrt::put_abi(_dqc)
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateDispatcherQueueController 失败", hr);
		return;
	}

	MagApp& app = MagApp::Get();

	while (true) {
		if (app.GetHwndHost()) {
			// 缩放时使用不同的消息循环
			bool quiting = !app.MessageLoop();

			_running = false;
			_isRunningChangedEvent(false);

			if (quiting) {
				return;
			}
		} else {
			if (_running) {
				// 缩放失败
				_running = false;
				_isRunningChangedEvent(false);
			}

			WaitMessage();

			MSG msg;
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
				if (msg.message == WM_QUIT) {
					return;
				}

				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}
}

void MagRuntime::_EnsureDispatcherQueue() const noexcept {
	while (!_dqc) {
		Sleep(1);
	}
}

}
