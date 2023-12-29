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

	_Dispatcher().TryEnqueue([hwndSrc, options(std::move(options))]() mutable {
		ScalingWindow::Get().Create(GetModuleHandle(nullptr), hwndSrc, std::move(options));
	});
}

void ScalingRuntime::ToggleOverlay() {
	if (!IsRunning()) {
		return;
	}

	_Dispatcher().TryEnqueue([]() {
		if (ScalingWindow& scalingWindow = ScalingWindow::Get()) {
			scalingWindow.ToggleOverlay();
		};
	});
}

void ScalingRuntime::Stop() {
	if (!IsRunning()) {
		return;
	}

	_Dispatcher().TryEnqueue([]() {
		ScalingWindow::Get().Destroy();
	});
}

void ScalingRuntime::_ScalingThreadProc() noexcept {
	winrt::init_apartment(winrt::apartment_type::single_threaded);

	{
		winrt::DispatcherQueueController dqc{ nullptr };
		HRESULT hr = CreateDispatcherQueueController(
			DispatcherQueueOptions{
				.dwSize = sizeof(DispatcherQueueOptions),
				.threadType = DQTYPE_THREAD_CURRENT
			},
			(PDISPATCHERQUEUECONTROLLER*)winrt::put_abi(dqc)
		);
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateDispatcherQueueController 失败", hr);
			return;
		}

		_dispatcher = dqc.DispatcherQueue();
		// 如果主线程正在等待则唤醒主线程
		_dispatcherInitialized.store(true, std::memory_order_release);
		_dispatcherInitialized.notify_one();
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

const winrt::DispatcherQueue& ScalingRuntime::_Dispatcher() noexcept {
	if (!_dispatcherInitializedCache) {
		_dispatcherInitialized.wait(false, std::memory_order_acquire);
		_dispatcherInitializedCache = true;
	}
	
	return _dispatcher;
}

}
