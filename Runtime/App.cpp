#include "pch.h"
#include "App.h"
#include "Utils.h"
#include "GraphicsCaptureFrameSource.h"
#include "GDIFrameSource.h"
#include "DwmSharedSurfaceFrameSource.h"
#include "LegacyGDIFrameSource.h"
#include "MagCallbackFrameSource.h"


extern std::shared_ptr<spdlog::logger> logger;

const UINT App::_WM_DESTORYHOST = RegisterWindowMessage(L"MAGPIE_WM_DESTORYHOST");


App::~App() {
	MagUninitialize();
	Windows::Foundation::Uninitialize();
}

bool App::Initialize(HINSTANCE hInst) {
	SPDLOG_LOGGER_INFO(logger, "正在初始化 App");

	_hInst = hInst;

	// 初始化 COM
	HRESULT hr = Windows::Foundation::Initialize(RO_INIT_MULTITHREADED);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_CRITICAL(logger, MakeComErrorMsg("初始化 COM 失败", hr));
		return false;
	}
	SPDLOG_LOGGER_INFO(logger, "已初始化 COM");

	// 注册主窗口类
	_RegisterHostWndClass();

	// 供隐藏光标和 MagCallback 抓取模式使用
	if (!MagInitialize()) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("MagInitialize 失败"));
	}

	SPDLOG_LOGGER_INFO(logger, "App 初始化成功");
	return true;
}

bool App::Run(
	HWND hwndSrc,
	const std::string& effectsJson,
	int captureMode,
	int frameRate,
	UINT flags
) {
	_hwndSrc = hwndSrc;
	_captureMode = captureMode;
	_frameRate = frameRate;
	_flags = flags;

	SPDLOG_LOGGER_INFO(logger, fmt::format("运行时参数：\n\thwndSrc：{}\n\tcaptureMode：{}\n\tadjustCursorSpeed：{}\n\tshowFPS：{}\n\tdisableRoundCorner：{}\n\tframeRate：{}\n\tdisableLowLatency：{}\n\tbreakpointMode：{}", (void*)hwndSrc, captureMode, IsAdjustCursorSpeed(), IsShowFPS(), IsDisableRoundCorner(), frameRate, IsDisableLowLatency(), IsBreakpointMode()));

	// 每次进入全屏都要重置
	_nextTimerId = 1;
	
	SetErrorMsg(ErrorMessages::GENERIC);

	_srcClientRect = Utils::GetClientScreenRect(_hwndSrc);
	if (_srcClientRect.right == 0 || _srcClientRect.bottom == 0) {
		SPDLOG_LOGGER_CRITICAL(logger, "获取源窗口客户区失败");
		return false;
	}

	SPDLOG_LOGGER_INFO(logger, fmt::format("源窗口客户区尺寸：{}x{}",
		_srcClientRect.right - _srcClientRect.left, _srcClientRect.bottom - _srcClientRect.top));

	if (!_CreateHostWnd()) {
		SPDLOG_LOGGER_CRITICAL(logger, "创建主窗口失败");
		return false;
	}

	_renderer.reset(new Renderer());
	if (!_renderer->Initialize()) {
		SPDLOG_LOGGER_CRITICAL(logger, "初始化 Renderer 失败，正在清理");
		DestroyWindow(_hwndHost);
		_Run();
		return false;
	}
	
	switch (captureMode) {
	case 0:
		_frameSource.reset(new GraphicsCaptureFrameSource());
		break;
	case 1:
		_frameSource.reset(new GDIFrameSource());
		break;
	case 2:
		_frameSource.reset(new DwmSharedSurfaceFrameSource());
		break;
	case 3:
		_frameSource.reset(new LegacyGDIFrameSource());
		break;
	case 4:
		_frameSource.reset(new MagCallbackFrameSource());
		break;
	default:
		SPDLOG_LOGGER_CRITICAL(logger, "未知的捕获模式，即将退出");
		DestroyWindow(_hwndHost);
		_Run();
		return false;
	}
	
	if (!_frameSource->Initialize()) {
		SPDLOG_LOGGER_CRITICAL(logger, "初始化 FrameSource 失败，即将退出");
		DestroyWindow(_hwndHost);
		_Run();
		return false;
	}

	if (!_renderer->InitializeEffectsAndCursor(effectsJson)) {
		SPDLOG_LOGGER_CRITICAL(logger, "初始化效果失败，即将退出");
		DestroyWindow(_hwndHost);
		_Run();
		return false;
	}

	// 合适时禁用窗口圆角
	bool roundCornerDisabled = false;
	if (IsDisableRoundCorner() && _frameSource->HasRoundCornerInWin11()) {
		const auto& version = Utils::GetOSVersion();
		bool isWin11 = Utils::CompareVersion(
			version.dwMajorVersion, version.dwMinorVersion,
			version.dwBuildNumber, 10, 0, 22000) >= 0;

		if (isWin11) {
			INT attr = DWMWCP_DONOTROUND;
			HRESULT hr = DwmSetWindowAttribute(hwndSrc, DWMWA_WINDOW_CORNER_PREFERENCE, &attr, sizeof(attr));
			if (FAILED(hr)) {
				SPDLOG_LOGGER_ERROR(logger, "禁用窗口圆角失败");
			} else {
				SPDLOG_LOGGER_INFO(logger, "已禁用窗口圆角");
				roundCornerDisabled = true;
			}
		}
	}

	_Run();

	if (roundCornerDisabled) {
		INT attr = DWMWCP_DEFAULT;
		HRESULT hr = DwmSetWindowAttribute(hwndSrc, DWMWA_WINDOW_CORNER_PREFERENCE, &attr, sizeof(attr));
		if (FAILED(hr)) {
			SPDLOG_LOGGER_INFO(logger, "取消禁用窗口圆角失败");
		} else {
			SPDLOG_LOGGER_INFO(logger, "已取消禁用窗口圆角");
		}
	}
	
	return true;
}

void App::_Run() {
	SPDLOG_LOGGER_INFO(logger, "开始接收窗口消息");

	while (true) {
		MSG msg;
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				// 释放资源
				_ReleaseResources();
				SPDLOG_LOGGER_INFO(logger, "主窗口已销毁");
				return;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		_renderer->Render();
	}
}

ComPtr<IWICImagingFactory2> App::GetWICImageFactory() {
    if (_wicImgFactory == nullptr) {
        HRESULT hr = CoCreateInstance(
            CLSID_WICImagingFactory,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&_wicImgFactory)
        );

		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("创建 WICImagingFactory 失败", hr));
			return nullptr;
		}
    }

    return _wicImgFactory;
}

bool App::RegisterTimer(UINT uElapse, std::function<void()> cb) {
	if (!SetTimer(_hwndHost, _nextTimerId, uElapse, nullptr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("SetTimer 失败"));
		return false;
	}

	++_nextTimerId;
	_timerCbs.emplace_back(std::move(cb));

	return true;
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
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("注册主窗口类失败"));
	} else {
		SPDLOG_LOGGER_INFO(logger, "已注册主窗口类");
	}
}

// 创建主窗口
bool App::_CreateHostWnd() {
	if (FindWindow(_HOST_WINDOW_CLASS_NAME, nullptr)) {
		SPDLOG_LOGGER_CRITICAL(logger, "已存在主窗口");
		return false;
	}

	RECT screenRect = Utils::GetScreenRect(_hwndSrc);
	_hostWndSize.cx = screenRect.right - screenRect.left;
	_hostWndSize.cy = screenRect.bottom - screenRect.top;
	_hwndHost = CreateWindowEx(
		(IsBreakpointMode() ? 0 : WS_EX_TOPMOST) | WS_EX_NOACTIVATE | WS_EX_LAYERED | WS_EX_TRANSPARENT,
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
		SPDLOG_LOGGER_CRITICAL(logger, MakeWin32ErrorMsg("创建主窗口失败"));
		return false;
	}

	SPDLOG_LOGGER_INFO(logger, fmt::format("主窗口尺寸：{}x{}", _hostWndSize.cx, _hostWndSize.cy));

	// 设置窗口不透明
	if (!SetLayeredWindowAttributes(_hwndHost, 0, 255, LWA_ALPHA)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("SetLayeredWindowAttributes 失败"));
	}

	if (!ShowWindow(_hwndHost, SW_NORMAL)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("ShowWindow 失败"));
	}

	SPDLOG_LOGGER_INFO(logger, "已创建主窗口");
	return true;
}

LRESULT App::_HostWndProcStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	return GetInstance()._HostWndProc(hWnd, message, wParam, lParam);
}


LRESULT App::_HostWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	if (message == _WM_DESTORYHOST) {
		SPDLOG_LOGGER_INFO(logger, "收到 MAGPIE_WM_DESTORYHOST 消息，即将销毁主窗口");
		DestroyWindow(_hwndHost);
		return 0;
	}

	switch (message) {
	case WM_DESTROY:
	{
		// 有两个退出路径：
		// 1. 前台窗口发生改变
		// 2. 收到_WM_DESTORYMAG 消息
		PostQuitMessage(0);
		return 0;
	}
	case WM_TIMER:
	{
		if (hWnd != _hwndHost || wParam <= 0 || wParam > _timerCbs.size()) {
			break;
		}

		_timerCbs[wParam - 1]();
		return 0;
	}
	default:
		break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

void App::_ReleaseResources() {
	_frameSource = nullptr;
	_renderer = nullptr;

	// 计时器资源在窗口销毁时自动释放
	_timerCbs.clear();
}
