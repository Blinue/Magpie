#include "pch.h"
#include "App.h"
#include "CommonSharedConstants.h"
#include "Logger.h"
#include <magnification.h>

static const UINT WM_MAGPIE_SCALINGCHANGED =
	RegisterWindowMessage(CommonSharedConstants::WM_MAGPIE_SCALINGCHANGED);
// 用于与主程序交互。wParam 的值:
// 0: Magpie 通知 TouchHelper 退出
static const UINT WM_MAGPIE_TOUCHHELPER =
	RegisterWindowMessage(CommonSharedConstants::WM_MAGPIE_TOUCHHELPER);

static constexpr UINT TIMER_ID = 1;

bool App::Initialzie() noexcept {
	if (!_CheckSingleInstance()) {
		Logger::Get().Error("_CheckSingleInstance 失败");
		_Uninitialize();
		return false;
	}

	if (!_CheckMagpieRunning()) {
		Logger::Get().Error("_CheckMagpieRunning 失败");
		_Uninitialize();
		return false;
	}

	if (MagInitialize()) {
		_isMagInitialized = true;
	} else {
		Logger::Get().Error("MagInitialize 失败");
		_Uninitialize();
		return false;
	}

	if (!_CreateMsgWindow()) {
		Logger::Get().Error("_CreateMsgWindow 失败");
		_Uninitialize();
		return false;
	}

    return true;
}

int App::Run() noexcept {
	MSG msg;
	while (true) {
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				_Uninitialize();
				return (int)msg.wParam;
			}

			DispatchMessage(&msg);
		}

		// 等待新消息或 Magpie 退出
		if (MsgWaitForMultipleObjectsEx(1, _hMagpieMutex.addressof(),
			INFINITE, QS_ALLINPUT, MWMO_INPUTAVAILABLE) == WAIT_OBJECT_0) {
			Logger::Get().Info("Magpie 已退出");
			_hMagpieMutex.ReleaseMutex();
			_Uninitialize();
			return 0;
		}
	}
}

bool App::_CheckSingleInstance() noexcept {
	bool alreadyExists = false;
	return _hSingleInstanceMutex.try_create(
		CommonSharedConstants::TOUCH_HELPER_SINGLE_INSTANCE_MUTEX_NAME,
		CREATE_MUTEX_INITIAL_OWNER,
		MUTEX_ALL_ACCESS,
		nullptr,
		&alreadyExists
	) && !alreadyExists;
}

bool App::_CheckMagpieRunning() noexcept {
	if (!_hMagpieMutex.try_create(CommonSharedConstants::SINGLE_INSTANCE_MUTEX_NAME)) {
		Logger::Get().Win32Error("CreateMutexEx 失败");
		return false;
	}

	if (wil::handle_wait(_hMagpieMutex.get(), 0)) {
		Logger::Get().Error("Magpie 未运行");
		_hMagpieMutex.ReleaseMutex();
		return false;
	}

	return true;
}

// 创建一个隐藏窗口用于接收广播消息
bool App::_CreateMsgWindow() noexcept {
	const HINSTANCE hInst = wil::GetModuleInstanceHandle();

	WNDCLASSEX wcex{
		.cbSize = sizeof(wcex),
		.lpfnWndProc = _WndProc,
		.hInstance = hInst,
		.lpszClassName = CommonSharedConstants::TOUCH_HELPER_WINDOW_CLASS_NAME
	};
	if (!RegisterClassEx(&wcex)) {
		Logger::Get().Win32Error("RegisterClassEx 失败");
		return false;
	}

	CreateWindow(
		CommonSharedConstants::TOUCH_HELPER_WINDOW_CLASS_NAME,
		nullptr,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		hInst,
		0
	);
	if (!_hwndMsg) {
		Logger::Get().Win32Error("CreateWindow 失败");
		return false;
	}

	return true;
}

LRESULT CALLBACK App::_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_NCCREATE) {
		Get()._hwndMsg.reset(hWnd);
	}

	return Get()._MessageHandler(msg, wParam, lParam);
}

LRESULT App::_MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
	if (msg == WM_MAGPIE_SCALINGCHANGED) {
		if (wParam == 0) {
			// 缩放结束
			_hwndScaling = NULL;
		} else if (wParam == 1) {
			// 缩放开始
			_hwndScaling = (HWND)lParam;
		}

		_UpdateInputTransform();

		// 用户拖动源窗口时使用计时器定期更新触控输入变换
		if (wParam == 3 && _isInputTransformEnabled) {
			if (SetTimer(_hwndMsg.get(), TIMER_ID, 20, nullptr)) {
				_isTimerOn = true;
			} else {
				Logger::Get().Win32Error("SetTimer 失败");
			}
		} else if (_isTimerOn) {
			if (KillTimer(_hwndMsg.get(), TIMER_ID)) {
				_isTimerOn = false;
			} else {
				Logger::Get().Win32Error("KillTimer 失败");
			}
		}

		return 0;
	} else if (msg == WM_MAGPIE_TOUCHHELPER) {
		if (wParam == 0) {
			// 退出
			_hwndMsg.reset();
		}

		return 0;
	}

	switch (msg) {
	case WM_CREATE:
	{
		// 防止消息被 UIPI 过滤
		ChangeWindowMessageFilterEx(_hwndMsg.get(), WM_MAGPIE_SCALINGCHANGED, MSGFLT_ADD, nullptr);
		ChangeWindowMessageFilterEx(_hwndMsg.get(), WM_MAGPIE_TOUCHHELPER, MSGFLT_ADD, nullptr);

		// 检查 Magpie 是否正在缩放，注意如果缩放窗口尚未显示视为没有缩放，此时
		// 缩放窗口正在初始化，会在完成后广播 WM_MAGPIE_SCALINGCHANGED 消息
		HWND hwndFound = FindWindow(CommonSharedConstants::SCALING_WINDOW_CLASS_NAME, nullptr);
		if (hwndFound && IsWindowVisible(hwndFound)) {
			_hwndScaling = hwndFound;
			_UpdateInputTransform();
		}

		return 0;
	}
	case WM_TIMER:
	{
		if (wParam == TIMER_ID) {
			_UpdateInputTransform(true);
		}
		return 0;
	}
	case WM_DESTROY:
	{
		_hwndScaling = NULL;
		_UpdateInputTransform();

		PostQuitMessage(0);
		break;
	}
	default:
		break;
	}

	return DefWindowProc(_hwndMsg.get(), msg, wParam, lParam);
}

void App::_UpdateInputTransform(bool onTimer) noexcept {
	if (_hwndScaling) {
		LONG srcLeft = (LONG)(INT_PTR)GetProp(_hwndScaling, L"Magpie.SrcTouchLeft");
		// 特殊值表示应禁用触控输入变换
		if (srcLeft != std::numeric_limits<LONG>::min()) {
			RECT srcRect{
				.left = srcLeft,
				.top = (LONG)(INT_PTR)GetProp(_hwndScaling, L"Magpie.SrcTouchTop"),
				.right = (LONG)(INT_PTR)GetProp(_hwndScaling, L"Magpie.SrcTouchRight"),
				.bottom = (LONG)(INT_PTR)GetProp(_hwndScaling, L"Magpie.SrcTouchBottom")
			};

			RECT destRect{
				.left = (LONG)(INT_PTR)GetProp(_hwndScaling, L"Magpie.DestTouchLeft"),
				.top = (LONG)(INT_PTR)GetProp(_hwndScaling, L"Magpie.DestTouchTop"),
				.right = (LONG)(INT_PTR)GetProp(_hwndScaling, L"Magpie.DestTouchRight"),
				.bottom = (LONG)(INT_PTR)GetProp(_hwndScaling, L"Magpie.DestTouchBottom")
			};

			if (MagSetInputTransform(TRUE, &srcRect, &destRect)) {
				_isInputTransformEnabled = true;

				// 拖动源窗口过程中避免记录太多日志
				if (!onTimer) {
					Logger::Get().Info(fmt::format("当前触控输入变换: {},{},{},{}->{},{},{},{}",
						srcRect.left, srcRect.top, srcRect.right, srcRect.bottom,
						destRect.left, destRect.top, destRect.right, destRect.bottom));
				}
			} else {
				Logger::Get().Win32Error("MagSetInputTransform 失败");
			}

			return;
		}
	}

	// 禁用触控输入变换
	if (_isInputTransformEnabled) {
		RECT unused{};
		if (MagSetInputTransform(FALSE, &unused, &unused)) {
			_isInputTransformEnabled = false;
			Logger::Get().Info("已禁用触控输入变换");
		} else {
			Logger::Get().Win32Error("MagSetInputTransform 失败");
		}
	}
}

void App::_Uninitialize() noexcept {
	_hwndMsg.reset();

	if (_isMagInitialized) {
		MagUninitialize();
		_isMagInitialized = false;
	}

	if (_hMagpieMutex) {
		_hMagpieMutex.reset();
	}

	if (_hSingleInstanceMutex) {
		_hSingleInstanceMutex.ReleaseMutex();
		_hSingleInstanceMutex.reset();
	}
}
