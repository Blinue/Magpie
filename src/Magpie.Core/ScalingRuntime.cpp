#include "pch.h"
#include "ScalingRuntime.h"
#include <dispatcherqueue.h>
#include "Logger.h"
#include "ScalingWindow.h"

namespace Magpie::Core {

ScalingRuntime::ScalingRuntime() : _scalingThread(std::bind(&ScalingRuntime::_ScalingThreadProc, this)) {
}

ScalingRuntime::~ScalingRuntime() {
	Stop();

	if (_scalingThread.joinable()) {
		const DWORD magWndThreadId = GetThreadId(_scalingThread.native_handle());
		// 持续尝试直到 _scalingThread 创建了消息队列
		while (!PostThreadMessage(magWndThreadId, WM_QUIT, 0, 0)) {
			Sleep(0);
		}
		_scalingThread.join();
	}
}

void ScalingRuntime::Start(HWND hwndSrc, ScalingOptions&& options) {
	HWND expected = NULL;
	if (!_hwndSrc.compare_exchange_strong(expected, hwndSrc, std::memory_order_relaxed)) {
		return;
	}

	_isRunningChangedEvent(true);

	_EnsureDispatcherQueue();
	_dqc.DispatcherQueue().TryEnqueue([hwndSrc, options(std::move(options))]() mutable {
		ScalingWindow::Get().Create(GetModuleHandle(nullptr), hwndSrc, std::move(options));
	});
}

void ScalingRuntime::ToggleOverlay() {
	if (!IsRunning()) {
		return;
	}

	_EnsureDispatcherQueue();
	_dqc.DispatcherQueue().TryEnqueue([]() {
		ScalingWindow::Get().ToggleOverlay();
	});
}

void ScalingRuntime::Stop() {
	if (!IsRunning()) {
		return;
	}

	_EnsureDispatcherQueue();
	_dqc.DispatcherQueue().TryEnqueue([]() {
		ScalingWindow::Get().Destroy();
	});
}

void ScalingRuntime::_ScalingThreadProc() noexcept {
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

	ScalingWindow& scalingWindow = ScalingWindow::Get();

	MSG msg;
	while (true) {
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				scalingWindow.Destroy();
				
				if (_hwndSrc.exchange(NULL, std::memory_order_relaxed)) {
					_isRunningChangedEvent(false);
				}

				return;
			}

			DispatchMessage(&msg);
		}

		if (scalingWindow) {
			scalingWindow.Render();
			MsgWaitForMultipleObjectsEx(0, nullptr, 1, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
		} else {
			if (_hwndSrc.exchange(NULL, std::memory_order_relaxed)) {
				// 缩放失败或立即退出缩放
				_isRunningChangedEvent(false);
			}
			WaitMessage();
		}
	}
}

void ScalingRuntime::_EnsureDispatcherQueue() const noexcept {
	while (!_dqc) {
		Sleep(0);
	}
}

}
