#include "pch.h"
#include "ScalingRuntime.h"
#include <dispatcherqueue.h>
#include "Logger.h"
#include "ScalingWindow.h"
#include "CommonSharedConstants.h"
#include "Win32Helper.h"

using namespace std::chrono;

namespace Magpie {

ScalingRuntime::ScalingRuntime() : _scalingThread(&ScalingRuntime::_ScalingThreadProc, this) {
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

bool ScalingRuntime::Start(HWND hwndSrc, ScalingOptions&& options) {
	if (!options.Prepare()) {
		return false;
	}

	_State expected = _State::Idle;
	if (!_state.compare_exchange_strong(
		expected, _State::Initializing, std::memory_order_relaxed)) {
		return false;
	}

	IsRunningChanged.Invoke(true, ScalingError::NoError);

	_Dispatcher().TryEnqueue([this, hwndSrc, options(std::move(options))]() mutable {
		options.Log();

		ScalingError error = ScalingWindow::Get().Create(hwndSrc, std::move(options));
		if (error == ScalingError::NoError) {
			_state.store(_State::Scaling, std::memory_order_relaxed);
		} else {
			// 缩放失败
			_state.store(_State::Idle, std::memory_order_relaxed);
			IsRunningChanged.Invoke(false, error);
		}
	});

	return true;
}

void ScalingRuntime::ToggleToolbarState() {
	if (!IsRunning()) {
		return;
	}

	_Dispatcher().TryEnqueue([]() {
		if (ScalingWindow& scalingWindow = ScalingWindow::Get()) {
			scalingWindow.ToggleToolbarState();
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
	GUITHREADINFO guiThreadInfo{ .cbSize = sizeof(GUITHREADINFO) };
	if (!GetGUIThreadInfo(GetWindowThreadProcessId(hwndSrc, nullptr), &guiThreadInfo)) {
		Logger::Get().Win32Error("GetGUIThreadInfo 失败");
		return -1;
	}

	return (guiThreadInfo.flags & GUI_INMOVESIZE) ? 0 : 1;
}

void ScalingRuntime::_ScalingThreadProc() noexcept {
#ifdef _DEBUG
	SetThreadDescription(GetCurrentThread(), L"Magpie-缩放线程");
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
	ScalingWindow::Dispatcher(_dispatcher);

	time_point<steady_clock> lastRenderTime;
	const milliseconds timeout(scalingWindow.Options().Is3DGameMode() ? 8 : 2);

	MSG msg;
	while (true) {
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				scalingWindow.Destroy();

				if (_state.exchange(_State::Idle, std::memory_order_relaxed) != _State::Idle) {
					IsRunningChanged.Invoke(false, ScalingError::NoError);
				}

				return;
			} else if (msg.message == CommonSharedConstants::WM_FRONTEND_RENDER && msg.hwnd == scalingWindow.Handle()) {
				// 缩放窗口收到 WM_FRONTEND_RENDER 将执行渲染
				lastRenderTime = steady_clock::now();
			}

			DispatchMessage(&msg);
		}

		if (_state.load(std::memory_order_relaxed) != _State::Scaling) {
			WaitMessage();
			continue;
		}

		if (scalingWindow) {
			const auto now = steady_clock::now();
			// 限制检测光标移动的频率
			nanoseconds rest = timeout - (now - lastRenderTime);
			if (rest.count() <= 0) {
				lastRenderTime = now;
				rest = timeout;
				scalingWindow.Render();
			}
			
			// 值为 1000000
			constexpr auto ratio = std::ratio_divide<std::milli, std::nano>().num;
			// 向上取整
			const DWORD restMs = DWORD((rest.count() + ratio - 1) / ratio);
			MsgWaitForMultipleObjectsEx(0, nullptr, restMs, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
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
