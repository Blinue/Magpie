#include "pch.h"
#include "ScalingRuntime.h"
#include "CommonSharedConstants.h"
#include "Logger.h"
#include "ScalingWindow.h"
#include "Win32Helper.h"
#include <dispatcherqueue.h>

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
	assert(!options.screenshotsDir.empty() && options.showToast && options.showError && options.save);

	_Dispatcher().TryEnqueue([this, hwndSrc, options(std::move(options))]() mutable {
		// 初始化时视为处于缩放状态
		_IsScaling(true);
		ScalingWindow::Get().Start(hwndSrc, std::move(options));
	});

	return true;
}

void ScalingRuntime::SwitchScalingState(bool isWindowedMode) {
	_Dispatcher().TryEnqueue([isWindowedMode]() {
		if (ScalingWindow& scalingWindow = ScalingWindow::Get()) {
			scalingWindow.SwitchScalingState(isWindowedMode);
		};
	});
}

void ScalingRuntime::SwitchToolbarState() {
	_Dispatcher().TryEnqueue([]() {
		if (ScalingWindow& scalingWindow = ScalingWindow::Get()) {
			scalingWindow.SwitchToolbarState();
		};
	});
}

void ScalingRuntime::Stop() {
	_Dispatcher().TryEnqueue([]() {
		ScalingWindow::Get().Stop();
	});
}

// 返回值:
// -1: 应取消缩放
// 0: 仍在调整中
// 1: 调整完毕
static int GetSrcRepositionState(HWND hwndSrc, bool allowScalingMaximized) noexcept {
	if (!IsWindow(hwndSrc)) {
		return -1;
	}

	if (Win32Helper::IsWindowHung(hwndSrc)) {
		return -1;
	}

	const UINT showCmd = Win32Helper::GetWindowShowCmd(hwndSrc);
	if (showCmd == SW_HIDE) {
		return -1;
	} else if (showCmd == SW_SHOWMAXIMIZED) {
		return allowScalingMaximized ? 1 : -1;
	} else if (showCmd == SW_SHOWMINIMIZED) {
		// 窗口最小化则继续等待
		return 0;
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

	MSG msg;
	while (true) {
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				scalingWindow.Stop();
				_IsScaling(false);
				return;
			} else if (msg.message == CommonSharedConstants::WM_FRONTEND_RENDER && msg.hwnd == scalingWindow.Handle()) {
				// 缩放窗口收到 WM_FRONTEND_RENDER 将执行渲染
				lastRenderTime = steady_clock::now();
			}

			DispatchMessage(&msg);
		}

		_IsScaling(scalingWindow);

		if (scalingWindow) {
			const auto now = steady_clock::now();
			// 限制检测光标移动的频率
			const milliseconds timeout(scalingWindow.Options().Is3DGameMode() ? 8 : 2);
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
				scalingWindow.SrcTracker().Handle(),
				scalingWindow.Options().RealIsAllowScalingMaximized()
			);
			if (state == 0) {
				// 等待调整完成
				MsgWaitForMultipleObjectsEx(0, nullptr, 10, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
			} else if (state == 1) {
				// 重新缩放
				ScalingWindow::Get().RestartAfterSrcRepositioned();
			} else {
				// 取消缩放
				ScalingWindow::Get().CleanAfterSrcMoved();
			}
		} else {
			lastRenderTime = {};
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

void ScalingRuntime::_IsScaling(bool value) {
	if (_isScaling.exchange(value, std::memory_order_relaxed) != value) {
		IsScalingChanged.Invoke(value);
	}
}

}
