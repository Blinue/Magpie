#include "pch.h"
#include <dispatcherqueue.h>
//#include "MagApp.h"
#include "ScalingRuntime.h"
#include "Logger.h"
#include "MagOptions.h"

namespace Magpie::Core {

ScalingRuntime::ScalingRuntime() : _scalingWndThread(std::bind(&ScalingRuntime::_MagWindThreadProc, this)) {
}

ScalingRuntime::~ScalingRuntime() {
	Stop();

	if (_scalingWndThread.joinable()) {
		DWORD magWndThreadId = GetThreadId(_scalingWndThread.native_handle());
		// 持续尝试直到 _scalingWndThread 创建了消息队列
		while (!PostThreadMessage(magWndThreadId, WM_QUIT, 0, 0)) {
			Sleep(1);
		}
		_scalingWndThread.join();
	}
}

void ScalingRuntime::Start(HWND hwndSrc, const MagOptions& options) {
	if (_running) {
		return;
	}

	_hwndSrc = hwndSrc;
	_running = true;
	_isRunningChangedEvent(true);

	_EnsureDispatcherQueue();
	_dqc.DispatcherQueue().TryEnqueue([this, hwndSrc, options(options)]() mutable {
		//MagApp::Get().Start(hwndSrc, std::move(options));
	});
}

void ScalingRuntime::ToggleOverlay() {
	if (!_running) {
		return;
	}

	_EnsureDispatcherQueue();
	_dqc.DispatcherQueue().TryEnqueue([]() {
		//MagApp::Get().ToggleOverlay();
	});
}

void ScalingRuntime::Stop() {
	if (!_running) {
		return;
	}

	_EnsureDispatcherQueue();
	_dqc.DispatcherQueue().TryEnqueue([]() {
		//MagApp::Get().Stop();
	});
}

void ScalingRuntime::_MagWindThreadProc() noexcept {
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

	while (true) {
		MSG msg;
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				return;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	/*MagApp& app = MagApp::Get();

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
	}*/
}

void ScalingRuntime::_EnsureDispatcherQueue() const noexcept {
	while (!_dqc) {
		Sleep(1);
	}
}

}
