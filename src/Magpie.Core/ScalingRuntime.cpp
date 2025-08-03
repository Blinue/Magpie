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
		ScalingWindow& scalingWindow = ScalingWindow::Get();
		// 如果正在缩放不做任何处理
		if (scalingWindow) {
			return;
		}

		if (scalingWindow.IsSrcRepositioning()) {
			scalingWindow.CleanAfterSrcRepositioned();
		}

		// 初始化时视为处于缩放状态
		_IsScaling(true);
		scalingWindow.Start(hwndSrc, std::move(options));
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

static std::optional<bool> IsSrcRepositioning(HWND hwndSrc) noexcept {
	if (!IsWindow(hwndSrc)) {
		Logger::Get().Info("源窗口已销毁");
		return std::nullopt;
	}

	// 窗口不可见或最小化则继续等待。注意 showCmd 不能准确判断窗口可见性，
	// 应使用 IsWindowVisible。
	if (!IsWindowVisible(hwndSrc)) {
		return true;
	}

	if (Win32Helper::IsWindowHung(hwndSrc)) {
		Logger::Get().Info("源窗口已挂起");
		return std::nullopt;
	}

	const UINT showCmd = Win32Helper::GetWindowShowCmd(hwndSrc);
	if (showCmd == SW_SHOWMAXIMIZED) {
		// 窗口最大化则尝试缩放，失败会显示错误消息
		return false;
	} else if (showCmd == SW_SHOWMINIMIZED) {
		return true;
	}

	// 检查源窗口是否正在调整大小或移动
	GUITHREADINFO guiThreadInfo{ .cbSize = sizeof(GUITHREADINFO) };
	if (!GetGUIThreadInfo(GetWindowThreadProcessId(hwndSrc, nullptr), &guiThreadInfo)) {
		Logger::Get().Win32Error("GetGUIThreadInfo 失败");
		return std::nullopt;
	}

	return bool(guiThreadInfo.flags & GUI_INMOVESIZE);
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
			} else if (msg.message == CommonSharedConstants::WM_FRONTEND_RENDER &&
				msg.hwnd == scalingWindow.Handle()) {
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
			std::optional<bool> repositioning =
				IsSrcRepositioning(scalingWindow.SrcTracker().Handle());
			if (repositioning.has_value()) {
				if (*repositioning) {
					// 等待调整完成
					MsgWaitForMultipleObjectsEx(0, nullptr, 10, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
				} else {
					// 重新缩放
					ScalingWindow::Get().RestartAfterSrcRepositioned();
				}
			} else {
				// 取消缩放
				ScalingWindow::Get().CleanAfterSrcRepositioned();
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
