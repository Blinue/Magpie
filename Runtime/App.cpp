#include "pch.h"
#include "App.h"
#include "Utils.h"
#include "WinRTFrameSource.h"


extern std::shared_ptr<spdlog::logger> logger;

const wchar_t* App::_errorMsg = ErrorMessages::GENERIC;
const UINT App::_WM_DESTORYHOST = RegisterWindowMessage(L"MAGPIE_WM_DESTORYHOST");


bool App::Initialize(
	HINSTANCE hInst,
	HWND hwndSrc,
	bool adjustCursorSpeed
) {
	_logger = logger;
	_hwndSrc = hwndSrc;
	_hInst = hInst;

	_adjustCursorSpeed = adjustCursorSpeed;

	SPDLOG_LOGGER_INFO(logger, "正在初始化 App");
	SetErrorMsg(ErrorMessages::GENERIC);

	// 确保只初始化一次
	static bool initalized = false;
	if (!initalized) {
		// 初始化 COM
		HRESULT hr = Windows::Foundation::Initialize(RO_INIT_MULTITHREADED);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_CRITICAL(logger, fmt::sprintf("初始化 COM 失败\n\tHRESULT：0x%X", hr));
			return false;
		}
		SPDLOG_LOGGER_INFO(logger, "已初始化 COM");

		// 注册主窗口类
		_RegisterHostWndClass();

		initalized = true;
	}

	if (!Utils::GetClientScreenRect(_hwndSrc, _srcClientRect)) {
		SPDLOG_LOGGER_CRITICAL(logger, "获取源窗口客户区失败");
		return false;
	}

	SPDLOG_LOGGER_INFO(logger, fmt::format("源窗口客户区尺寸：{}x{}",
		_srcClientRect.right - _srcClientRect.left, _srcClientRect.bottom - _srcClientRect.top));

	if (!_CreateHostWnd()) {
		SPDLOG_LOGGER_INFO(logger, "创建主窗口失败");
		return false;
	}

	_renderer.reset(new Renderer());
	if (!_renderer->Initialize()) {
		SPDLOG_LOGGER_INFO(logger, "初始化 Renderer 失败，正在清理");
		DestroyWindow(_hwndHost);
		Run();
		return false;
	}
	
	_frameSource.reset(new WinRTFrameSource());
	if (!_frameSource->Initialize()) {
		SPDLOG_LOGGER_INFO(logger, "初始化 WinRTFrameSource 失败，正在清理");
		DestroyWindow(_hwndHost);
		Run();
		return false;
	}

	if (!_renderer->InitializeEffects()) {
		SPDLOG_LOGGER_INFO(logger, "初始化效果失败，正在清理");
		DestroyWindow(_hwndHost);
		Run();
		return false;
	}

	_cursorManager.reset(new CursorManager());
	if (!_cursorManager->Initialize()) {
		SPDLOG_LOGGER_INFO(logger, "初始化 CursorManager 失败，正在清理");
		DestroyWindow(_hwndHost);
		Run();
		return false;
	}

	SPDLOG_LOGGER_INFO(logger, "App 初始化成功");
	return true;
}

void App::Run() {
	SPDLOG_LOGGER_INFO(_logger, "开始接收窗口消息");

	while (true) {
		MSG msg;
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				// 释放资源
				_ReleaseResources();
				SPDLOG_LOGGER_INFO(_logger, "主窗口已销毁");
				return;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		_renderer->Render();
	}
}

// 注册主窗口类
void App::_RegisterHostWndClass() const {
	WNDCLASSEX wcex = {};
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = _HostWndProcStatic;
	wcex.hInstance = _hInst;
	wcex.lpszClassName = _HOST_WINDOW_CLASS_NAME;

	if (!RegisterClassEx(&wcex)) {
		// 忽略此错误，因为可能是重复注册产生的错误
		SPDLOG_LOGGER_ERROR(_logger, fmt::format("注册主窗口类失败\n\tLastErrorCode：{}", GetLastError()));
	} else {
		SPDLOG_LOGGER_INFO(_logger, "已注册主窗口类");
	}
}

// 创建主窗口
bool App::_CreateHostWnd() {
	if (FindWindow(_HOST_WINDOW_CLASS_NAME, nullptr)) {
		SPDLOG_LOGGER_CRITICAL(_logger, "已存在主窗口");
		return false;
	}

	RECT screenRect = Utils::GetScreenRect(_hwndSrc);
	_hostWndSize.cx = screenRect.right - screenRect.left;
	_hostWndSize.cy = screenRect.bottom - screenRect.top;
	_hwndHost = CreateWindowEx(
		WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_LAYERED | WS_EX_TRANSPARENT,
		_HOST_WINDOW_CLASS_NAME,
		NULL, WS_CLIPCHILDREN | WS_POPUP | WS_VISIBLE,
		screenRect.left,
		screenRect.top,
		_hostWndSize.cx,
		_hostWndSize.cy,
		NULL,
		NULL,
		_hInst,
		NULL
	);
	if (!_hwndHost) {
		SPDLOG_LOGGER_CRITICAL(_logger, fmt::format("创建主窗口失败\n\tLastErrorCode：{}", GetLastError()));
		return false;
	}

	SPDLOG_LOGGER_INFO(_logger, fmt::format("主窗口尺寸：{}x{}", _hostWndSize.cx, _hostWndSize.cy));

	// 设置窗口不透明
	if (!SetLayeredWindowAttributes(_hwndHost, 0, 255, LWA_ALPHA)) {
		SPDLOG_LOGGER_ERROR(_logger, fmt::format("SetLayeredWindowAttributes 失败\n\tLastErrorCode：{}", GetLastError()));
	}

	// 取消置顶，这样可以使该窗口在最前
	if (!ShowWindow(_hwndHost, SW_NORMAL)) {
		SPDLOG_LOGGER_ERROR(_logger, fmt::format("ShowWindow 失败\n\tLastErrorCode：{}", GetLastError()));
	}
	DWORD style = GetWindowStyle(_hwndHost);
	if (!style) {
		SPDLOG_LOGGER_ERROR(_logger, fmt::format("GetWindowStyle 失败\n\tLastErrorCode：{}", GetLastError()));
	}
	style &= ~WS_EX_TOPMOST;
	if (!SetWindowLong(_hwndHost, GWL_STYLE, style)) {
		SPDLOG_LOGGER_ERROR(_logger, fmt::format("SetWindowLong 失败\n\tLastErrorCode：{}", GetLastError()));
	}

	SPDLOG_LOGGER_INFO(_logger, "已创建主窗口");
	return true;
}

LRESULT App::_HostWndProcStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	return GetInstance()._HostWndProc(hWnd, message, wParam, lParam);
}


LRESULT App::_HostWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	if (message == _WM_DESTORYHOST) {
		SPDLOG_LOGGER_INFO(_logger, "收到 MAGPIE_WM_DESTORYHOST 消息，即将销毁主窗口");
		DestroyWindow(_hwndHost);
		return 0;
	}
	if (message == WM_DESTROY) {
		// 有两个退出路径：
		// 1. 前台窗口发生改变
		// 2. 收到_WM_DESTORYMAG 消息
		PostQuitMessage(0);
		return 0;
	} else {
		return DefWindowProc(hWnd, message, wParam, lParam);
		/*auto [resolved, rt] = _renderManager->WndProc(hWnd, message, wParam, lParam);

		if (resolved) {
			return rt;
		} else {
			return DefWindowProc(hWnd, message, wParam, lParam);
		}*/
	}

	return 0;
}

void App::_ReleaseResources() {
	_cursorManager = nullptr;
	_frameSource = nullptr;
	_renderer = nullptr;
}

