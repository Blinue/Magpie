#include "pch.h"
#include "ScalingRuntime.h"
#include <dispatcherqueue.h>
#include "Logger.h"
#include "ScalingWindow.h"

namespace Magpie {

ScalingRuntime::ScalingRuntime() :
	_scalingThread(std::bind_front(&ScalingRuntime::_ScalingThreadProc, this)) {
}

ScalingRuntime::~ScalingRuntime() {
	Stop();

	if (_scalingThread.joinable()) {
		const HANDLE hScalingThread = _scalingThread.native_handle();

		if (!wil::handle_wait(hScalingThread, 0)) {
			const DWORD threadId = GetThreadId(hScalingThread);
			// 持续尝试直到 _scalingThread 创建了消息队列
			while (!PostThreadMessage(threadId, WM_QUIT, 0, 0)) {
				if (wil::handle_wait(hScalingThread, 1)) {
					break;
				}
			}

			// 等待缩放线程退出，在此期间必须处理消息队列，否则缩放线程调用
			// SetWindowLongPtr 会导致死锁
			while (true) {
				MSG msg;
				while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}

				if (MsgWaitForMultipleObjectsEx(1, &hScalingThread,
					INFINITE, QS_ALLINPUT, MWMO_INPUTAVAILABLE) == WAIT_OBJECT_0) {
					// WAIT_OBJECT_0 表示缩放线程已退出
					// WAIT_OBJECT_0 + 1 表示有新消息
					break;
				}
			}
		}
		
		_scalingThread.join();
	}
}

void ScalingRuntime::Start(HWND hwndSrc, ScalingOptions&& options) {
	_State expected = _State::Idle;
	if (!_state.compare_exchange_strong(
		expected, _State::Initializing, std::memory_order_relaxed)) {
		return;
	}

	IsRunningChanged.Invoke(true, ScalingError::NoError);

	_Dispatcher().TryEnqueue([this, dispatcher(_Dispatcher()), hwndSrc, options(std::move(options))]() mutable {
		ScalingError error = ScalingWindow::Get().Create(dispatcher, hwndSrc, std::move(options));
		if (error == ScalingError::NoError) {
			_state.store(_State::Scaling, std::memory_order_relaxed);
		} else {
			// 缩放失败
			_state.store(_State::Idle, std::memory_order_relaxed);
			IsRunningChanged.Invoke(false, error);
		}
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
		// 消息循环会更改 _state
		ScalingWindow& scalingWindow = ScalingWindow::Get();
		if (scalingWindow.IsSrcRepositioning()) {
			scalingWindow.CleanAfterSrcRepositioned();
		} else {
			scalingWindow.Destroy();
		}
	});
}

// 返回值:
// -1: 应取消缩放
// 0: 仍在调整中
// 1: 调整完毕
static int GetSrcRepositionState(HWND hwndSrc, bool allowScalingMaximized) noexcept {
	if (!IsWindow(hwndSrc) || GetForegroundWindow() != hwndSrc) {
		return -1;
	}

	if (UINT showCmd = Win32Helper::GetWindowShowCmd(hwndSrc); showCmd != SW_NORMAL) {
		if (showCmd != SW_SHOWMAXIMIZED || !allowScalingMaximized) {
			return -1;
		}
	}

	// 检查源窗口是否正在调整大小或移动
	GUITHREADINFO guiThreadInfo{
		.cbSize = sizeof(GUITHREADINFO)
	};
	if (!GetGUIThreadInfo(GetWindowThreadProcessId(hwndSrc, nullptr), &guiThreadInfo)) {
		Logger::Get().Win32Error("GetGUIThreadInfo 失败");
		return -1;
	}

	return (guiThreadInfo.flags & GUI_INMOVESIZE) ? 0 : 1;
}

void ScalingRuntime::_ScalingThreadProc() noexcept {
#ifdef _DEBUG
	SetThreadDescription(GetCurrentThread(), L"Magpie 缩放线程");
#endif

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

				if (_state.exchange(_State::Idle, std::memory_order_relaxed) != _State::Idle) {
					IsRunningChanged.Invoke(false, ScalingError::NoError);
				}

				return;
			}

			DispatchMessage(&msg);
		}

		if (_state.load(std::memory_order_relaxed) != _State::Scaling) {
			WaitMessage();
			continue;
		}

		if (scalingWindow) {
			scalingWindow.Render();
			MsgWaitForMultipleObjectsEx(0, nullptr, 1, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
		} else if (scalingWindow.IsSrcRepositioning()) {
			const int state = GetSrcRepositionState(
				scalingWindow.SrcInfo().Handle(),
				scalingWindow.Options().IsAllowScalingMaximized()
			);
			if (state == 0) {
				// 等待调整完成
				MsgWaitForMultipleObjectsEx(0, nullptr, 10, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
			} else if (state == 1) {
				// 重新缩放
				ScalingWindow::Get().RecreateAfterSrcRepositioned();
			} else {
				// 取消缩放
				ScalingWindow::Get().CleanAfterSrcRepositioned();
			}
		} else {
			// 缩放结束
			_state.store(_State::Idle, std::memory_order_relaxed);
			IsRunningChanged.Invoke(false, scalingWindow.RuntimeError());
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
