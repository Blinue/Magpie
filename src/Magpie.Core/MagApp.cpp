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
#include "GPUTimer.h"


namespace Magpie::Core {

static constexpr const wchar_t* HOST_WINDOW_CLASS_NAME = L"Window_Magpie_967EB565-6F73-4E94-AE53-00CC42592A22";
static constexpr const wchar_t* DDF_WINDOW_CLASS_NAME = L"Window_Magpie_C322D752-C866-4630-91F5-32CB242A8930";


static LRESULT DDFWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_DESTROY) {error
		return 0;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

static LRESULT CALLBACK LowLevelKeyboardProc(
	_In_ int    nCode,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam
) {
	if (nCode != HC_ACTION || wParam != WM_KEYDOWN) {
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	}

	KBDLLHOOKSTRUCT* info = (KBDLLHOOKSTRUCT*)lParam;
	if (info->vkCode == VK_SNAPSHOT) {
		([]()->winrt::fire_and_forget {
			MagApp& app = MagApp::Get();

			if (!app.GetOptions().IsDrawCursor()) {
				co_return;
			}

			// 暂时隐藏光标
			app.GetCursorManager().Hide();
			app.GetRenderer().Render(true);

			winrt::DispatcherQueue dispatcher = app.Dispatcher();

			co_await std::chrono::milliseconds(400);
			co_await dispatcher;

			app.GetCursorManager().Show();
		})();
	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

MagApp::MagApp() {
	_hInst = GetModuleHandle(nullptr);
}

MagApp::~MagApp() {}

bool MagApp::Run(HWND hwndSrc, MagOptions&& options, winrt::DispatcherQueue const& dispatcher) {
	_dispatcher = dispatcher;
	_hwndSrc = hwndSrc;
	_options = options;

	// 模拟独占全屏
	// 必须在主窗口创建前，否则 SHQueryUserNotificationState 可能返回 QUNS_BUSY 而不是 QUNS_RUNNING_D3D_FULL_SCREEN
	std::unique_ptr<ExclModeHack> exclMode;
	if (MagApp::Get().GetOptions().IsSimulateExclusiveFullscreen()) {
		exclMode = std::make_unique<ExclModeHack>();
	};

	_RegisterWndClasses();

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

	_renderer.reset(new Renderer());
	if (!_renderer->Initialize()) {
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

	if (_options.IsDisableDirectFlip() && !_options.IsDebugMode()) {
		// 在此处创建的 DDF 窗口不会立刻显示
		if (!_DisableDirectFlip()) {
			Logger::Get().Error("_DisableDirectFlip 失败");
		}
	}

	_hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);

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

	_hwndDDF = NULL;
	_hwndHost = NULL;
}

void MagApp::ToggleOverlay() {
	_renderer->SetUIVisibility(!_renderer->IsUIVisiable());
}

uint32_t MagApp::RegisterWndProcHandler(std::function<std::optional<LRESULT>(HWND, UINT, WPARAM, LPARAM)> handler) noexcept {
	uint32_t id = _nextWndProcHandlerID++;
	_wndProcHandlers.emplace_back(std::move(handler), id);
	return id;
}

bool MagApp::UnregisterWndProcHandler(uint32_t id) noexcept {
	if (id == 0) {
		return false;
	}

	// 从后向前查找，因为后注册的回调更可能先取消注册
	for (int i = (int)_wndProcHandlers.size() - 1; i >= 0; --i) {
		if (_wndProcHandlers[i].second == id) {
			_wndProcHandlers.erase(_wndProcHandlers.begin() + i);
			return true;
		}
	}

	return false;
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

		// 第二帧（等待时或完成后）显示 DDF 窗口
		// 如果在 Run 中创建会有短暂的灰屏
		// 选择第二帧的原因：当 GetFrameCount() 返回 1 时第一帧可能处于等待状态而没有渲染，见 Renderer::Render()
		if (_renderer->GetGPUTimer().GetFrameCount() == 2 && _hwndDDF) {
			ShowWindow(_hwndDDF, SW_NORMAL);

			if (!SetWindowPos(_hwndDDF, _hwndHost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOREDRAW)) {
				Logger::Get().Win32Error("SetWindowPos 失败");
			}
		}
	}
}

void MagApp::_RegisterWndClasses() const {
	static bool registered = false;
	if (!registered) {
		registered = true;

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
}

static BOOL CALLBACK MonitorEnumProc(HMONITOR, HDC, LPRECT monitorRect, LPARAM data) {
	RECT* params = (RECT*)data;

	if (Win32Utils::CheckOverlap(params[0], *monitorRect)) {
		UnionRect(&params[1], monitorRect, &params[1]);
	}

	return TRUE;
}

static bool CalcHostWndRect(HWND hWnd, MultiMonitorUsage multiMonitorUsage, RECT& result) {
	switch (multiMonitorUsage) {
	case MultiMonitorUsage::Nearest:
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
	case MultiMonitorUsage::Intersected:
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
	case MultiMonitorUsage::All:
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

	if (!CalcHostWndRect(_hwndSrc, _options.multiMonitorUsage, _hostWndRect)) {
		Logger::Get().Error("CalcHostWndRect 失败");
		return false;
	}

	_hwndHost = CreateWindowEx(
		(_options.IsDebugMode() ? 0 : WS_EX_TOPMOST) | WS_EX_NOACTIVATE | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
		HOST_WINDOW_CLASS_NAME,
		NULL,	// 标题为空，否则会被添加新配置页面列为候选窗口
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
	if (!SetLayeredWindowAttributes(_hwndHost, 0, _options.IsDisableDirectFlip() ? 254 : 255, LWA_ALPHA)) {
		Logger::Get().Win32Error("SetLayeredWindowAttributes 失败");
	}

	return true;
}

bool MagApp::_InitFrameSource() {
	switch (_options.captureMode) {
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
	// 没有显式关闭 DirectFlip 的方法
	// 将全屏窗口设为稍微透明，以灰色全屏窗口为背景
	_hwndDDF = CreateWindowEx(
		WS_EX_NOACTIVATE | WS_EX_LAYERED | WS_EX_TRANSPARENT,
		DDF_WINDOW_CLASS_NAME,
		NULL,
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

	if (!_hwndDDF) {
		Logger::Get().Win32Error("创建 DDF 窗口失败");
		return false;
	}

	// 设置窗口不透明
	if (!SetLayeredWindowAttributes(_hwndDDF, 0, 255, LWA_ALPHA)) {
		Logger::Get().Win32Error("SetLayeredWindowAttributes 失败");
	}

	if (_frameSource->IsScreenCapture()) {
		const RTL_OSVERSIONINFOW& version = Win32Utils::GetOSVersion();
		if (Utils::CompareVersion(version.dwMajorVersion, version.dwMinorVersion, version.dwBuildNumber, 10, 0, 19041) >= 0) {
			// 使 DDF 窗口无法被捕获到
			if (!SetWindowDisplayAffinity(_hwndDDF, WDA_EXCLUDEFROMCAPTURE)) {
				Logger::Get().Win32Error("SetWindowDisplayAffinity 失败");
			}
		}
	}

	Logger::Get().Info("已创建 DDF 主窗口");
	return true;
}

LRESULT MagApp::_HostWndProcStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	return Get()._HostWndProc(hWnd, message, wParam, lParam);
}

LRESULT MagApp::_HostWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	// 以反向调用回调
	for (auto it = _wndProcHandlers.rbegin(); it != _wndProcHandlers.rend(); ++it) {
		const auto& result = it->first(hWnd, message, wParam, lParam);
		if (result.has_value()) {
			return result.value();
		}
	}

	switch (message) {
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

void MagApp::_OnQuit() {
	if (_hKeyboardHook) {
		UnhookWindowsHookEx(_hKeyboardHook);
		_hKeyboardHook = NULL;
	}

	// 释放资源
	_cursorManager = nullptr;
	_renderer = nullptr;
	_frameSource = nullptr;
	_deviceResources = nullptr;

	_nextWndProcHandlerID = 1;
	_wndProcHandlers.clear();

	_dispatcher = nullptr;
	_hwndSrc = NULL;
}

}
