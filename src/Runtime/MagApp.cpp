#include "pch.h"
#include "MagApp.h"
#include "Logger.h"
#include "Win32Utils.h"
#include "ExclModeHack.h"
#include "DeviceResources.h"
#include "GraphicsCaptureFrameSource.h"
#include "DesktopDuplicationFrameSource.h"
#include "GDIFrameSource.h"
#include "DwmSharedSurfaceFrameSource.h"
#include "StrUtils.h"
#include "CursorManager.h"
#include "Renderer.h"


static constexpr const wchar_t* HOST_WINDOW_CLASS_NAME = L"Window_Magpie_967EB565-6F73-4E94-AE53-00CC42592A22";
static constexpr const wchar_t* DDF_WINDOW_CLASS_NAME = L"Window_Magpie_C322D752-C866-4630-91F5-32CB242A8930";
static constexpr const wchar_t* HOST_WINDOW_TITLE = L"Magpie_Host";


LRESULT DDFWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_DESTROY) {
		return 0;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

MagApp::MagApp() {}

MagApp::~MagApp() {}

bool MagApp::Run(HWND hwndSrc, winrt::Magpie::Runtime::MagSettings const& settings) {
    _hwndSrc = hwndSrc;
    _settings = settings;

	_hInst = GetModuleHandle(nullptr);

	// 模拟独占全屏
	// 必须在主窗口创建前，否则 SHQueryUserNotificationState 可能返回 QUNS_BUSY 而不是 QUNS_RUNNING_D3D_FULL_SCREEN
	ExclModeHack exclMode;

	static bool initialized = false;
	if (!initialized) {
		initialized = true;

		WNDCLASSEX wcex = {};
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.lpfnWndProc = _HostWndProcStatic;
		wcex.hInstance = _hInst;
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex.lpszClassName = HOST_WINDOW_CLASS_NAME;

		if (!RegisterClassEx(&wcex)) {
			// 忽略此错误，因为可能是重复注册产生的错误
			Logger::Get().Win32Error("注册主窗口类失败");
		} else {
			Logger::Get().Info("已注册主窗口类");
		}

		wcex.lpfnWndProc = DDFWndProc;
		wcex.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
		wcex.lpszClassName = DDF_WINDOW_CLASS_NAME;

		if (!RegisterClassEx(&wcex)) {
			Logger::Get().Win32Error("注册 DDF 窗口类失败");
		} else {
			Logger::Get().Info("已注册 DDF 窗口类");
		}
	}
	
	if (!_CreateHostWnd()) {
		Logger::Get().Error("创建主窗口失败");
		_OnQuit();
		return false;
	}

	_deviceResources.reset(new DeviceResources());
	if (!_deviceResources->Initialize()) {
		Logger::Get().Error("初始化 DeviceResources 失败");
		Stop();
		_RunMessageLoop();
		return false;
	}

	if (!_InitFrameSource()) {
		Logger::Get().Critical("_InitFrameSource 失败");
		Stop();
		_RunMessageLoop();
		return false;
	}

	const char* effectsJson = R"(
[
  {
    "effect": "FSR_EASU",
    "scale": [ -1, -1 ]
  },
  {
    "effect": "FSR_RCAS"
  }
]
)";

	_renderer.reset(new Renderer());
	if (!_renderer->Initialize(effectsJson)) {
		Logger::Get().Critical("初始化 Renderer 失败");
		Stop();
		_RunMessageLoop();
		return false;
	}

	_cursorManager.reset(new CursorManager());
	if (!_cursorManager->Initialize()) {
		Logger::Get().Critical("初始化 CursorManager 失败");
		Stop();
		_RunMessageLoop();
		return false;
	}

	/*if (_config->IsDisableDirectFlip() && !_config->IsBreakpointMode()) {
		// 在此处创建的 DDF 窗口不会立刻显示
		if (!_DisableDirectFlip()) {
			Logger::Get().Error("_DisableDirectFlip 失败");
		}
	}*/

	ShowWindow(_hwndHost, SW_NORMAL);

	_RunMessageLoop();

    return false;
}

void MagApp::Stop() {
    if (_hwndDDF) {
        DestroyWindow(_hwndDDF);
    }
    if (_hwndHost) {
        DestroyWindow(_hwndHost);
    }
}

void MagApp::ToggleOverlay() {
	_renderer->SetUIVisibility(!_renderer->IsUIVisiable());
}

winrt::com_ptr<IWICImagingFactory2> MagApp::GetWICImageFactory() {
	static winrt::com_ptr<IWICImagingFactory2> wicImgFactory;

	if (wicImgFactory == nullptr) {
		HRESULT hr = CoCreateInstance(
			CLSID_WICImagingFactory,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(wicImgFactory.put())
		);

		if (FAILED(hr)) {
			Logger::Get().ComError("创建 WICImagingFactory 失败", hr);
			return nullptr;
		}
	}

	return wicImgFactory;
}

UINT MagApp::RegisterWndProcHandler(std::function<std::optional<LRESULT>(HWND, UINT, WPARAM, LPARAM)> handler) {
	UINT id = _nextWndProcHandlerID++;
	return _wndProcHandlers.emplace(id, handler).second ? id : 0;
}

void MagApp::UnregisterWndProcHandler(UINT id) {
	_wndProcHandlers.erase(id);
}

void MagApp::_RunMessageLoop() {
	while (true) {
		MSG msg;
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				_OnQuit();
				return;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		_renderer->Render();

		//// 第二帧（等待时或完成后）显示 DDF 窗口
		//// 如果在 Run 中创建会有短暂的灰屏
		//// 选择第二帧的原因：当 GetFrameCount() 返回 1 时第一帧可能处于等待状态而没有渲染，见 Renderer::Render()
		//if (_renderer->GetGPUTimer().GetFrameCount() == 2 && _hwndDDF) {
		//	ShowWindow(_hwndDDF, SW_NORMAL);

		//	if (!SetWindowPos(_hwndDDF, _hwndHost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOREDRAW)) {
		//		Logger::Get().Win32Error("SetWindowPos 失败");
		//	}
		//}
	}
}

void MagApp::_RegisterWndClasses() const {
}

static BOOL CALLBACK MonitorEnumProc(HMONITOR, HDC, LPRECT monitorRect, LPARAM data) {
	RECT* params = (RECT*)data;

	if (Win32Utils::CheckOverlap(params[0], *monitorRect)) {
		UnionRect(&params[1], monitorRect, &params[1]);
	}

	return TRUE;
}

static bool CalcHostWndRect(HWND hWnd, UINT multiMonitorMode, RECT& result) {
	switch (multiMonitorMode) {
	case 0:
	{
		// 使用距离源窗口最近的显示器
		HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		if (!hMonitor) {
			Logger::Get().Win32Error("MonitorFromWindow 失败");
			return false;
		}

		MONITORINFO mi{};
		mi.cbSize = sizeof(mi);
		if (!GetMonitorInfo(hMonitor, &mi)) {
			Logger::Get().Win32Error("GetMonitorInfo 失败");
			return false;
		}
		result = mi.rcMonitor;

		break;
	}
	case 1:
	{
		// 使用源窗口跨越的所有显示器

		// [0] 存储源窗口坐标，[1] 存储计算结果
		RECT params[2]{};

		if (!Win32Utils::GetWindowFrameRect(hWnd, params[0])) {
			Logger::Get().Error("GetWindowFrameRect 失败");
			return false;
		}

		if (!EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)&params)) {
			Logger::Get().Win32Error("EnumDisplayMonitors 失败");
			return false;
		}

		result = params[1];
		if (result.right - result.left <= 0 || result.bottom - result.top <= 0) {
			Logger::Get().Error("计算主窗口坐标失败");
			return false;
		}

		break;
	}
	case 2:
	{
		// 使用所有显示器（Virtual Screen）
		int vsWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
		int vsHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
		int vsX = GetSystemMetrics(SM_XVIRTUALSCREEN);
		int vsY = GetSystemMetrics(SM_YVIRTUALSCREEN);
		result = { vsX, vsY, vsX + vsWidth, vsY + vsHeight };

		break;
	}
	default:
		return false;
	}

	return true;
}

// 创建主窗口
bool MagApp::_CreateHostWnd() {
	if (FindWindow(HOST_WINDOW_CLASS_NAME, nullptr)) {
		Logger::Get().Error("已存在主窗口");
		return false;
	}

	if (!CalcHostWndRect(_hwndSrc, 0, _hostWndRect)) {
		Logger::Get().Error("CalcHostWndRect 失败");
		return false;
	}

	_hwndHost = CreateWindowEx(
		/*WS_EX_TOPMOST |*/ WS_EX_NOACTIVATE | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
		HOST_WINDOW_CLASS_NAME,
		HOST_WINDOW_TITLE,
		WS_POPUP,
		_hostWndRect.left,
		_hostWndRect.top,
		_hostWndRect.right - _hostWndRect.left,
		_hostWndRect.bottom - _hostWndRect.top,
		NULL,
		NULL,
		_hInst,
		NULL
	);
	if (!_hwndHost) {
		Logger::Get().Win32Error("创建主窗口失败");
		return false;
	}

	Logger::Get().Info(fmt::format("主窗口尺寸：{}x{}",
		_hostWndRect.right - _hostWndRect.left, _hostWndRect.bottom - _hostWndRect.top));

	// 设置窗口不透明
	// 不完全透明时可关闭 DirectFlip
	if (!SetLayeredWindowAttributes(_hwndHost, 0, 255, LWA_ALPHA)) {
		Logger::Get().Win32Error("SetLayeredWindowAttributes 失败");
	}

	return true;
}

bool MagApp::_InitFrameSource() {
	using winrt::Magpie::Runtime::CaptureMode;

	switch (_settings.CaptureMode()) {
	case CaptureMode::GraphicsCapture:
		_frameSource.reset(new GraphicsCaptureFrameSource());
		break;
	case CaptureMode::DesktopDuplication:
		_frameSource.reset(new DesktopDuplicationFrameSource());
		break;
	case CaptureMode::GDI:
		_frameSource.reset(new GDIFrameSource());
		break;
	case CaptureMode::DwmSharedSurface:
		_frameSource.reset(new DwmSharedSurfaceFrameSource());
		break;
	default:
		Logger::Get().Critical("未知的捕获模式");
		return false;
	}

	Logger::Get().Info(StrUtils::Concat("当前捕获模式：", _frameSource->GetName()));

	if (!_frameSource->Initialize()) {
		Logger::Get().Critical("初始化 FrameSource 失败");
		return false;
	}

	const RECT& frameRect = _frameSource->GetSrcFrameRect();
	Logger::Get().Info(fmt::format("源窗口尺寸：{}x{}",
		frameRect.right - frameRect.left, frameRect.bottom - frameRect.top));

	return true;
}

bool MagApp::_DisableDirectFlip() {
    return false;
}

LRESULT MagApp::_HostWndProcStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	return Get()._HostWndProc(hWnd, message, wParam, lParam);
}

LRESULT MagApp::_HostWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	// 以反向调用回调
	for (auto it = _wndProcHandlers.rbegin(); it != _wndProcHandlers.rend(); ++it) {
		const auto& result = it->second(hWnd, message, wParam, lParam);
		if (result.has_value()) {
			return result.value();
		}
	}

	switch (message) {
	case WM_DESTROY:
		// 有两个退出路径：
		// 1. 前台窗口发生改变
		// 2. 收到_WM_DESTORYMAG 消息
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

void MagApp::_OnQuit() {
	// 释放资源
	_cursorManager = nullptr;
	_renderer = nullptr;
	_frameSource = nullptr;
	_deviceResources = nullptr;
	_settings = nullptr;

	_nextWndProcHandlerID = 1;
	_wndProcHandlers.clear();
}
