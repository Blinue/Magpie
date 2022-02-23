#include "pch.h"
#include "App.h"
#include "Utils.h"
#include "GraphicsCaptureFrameSource.h"
#include "GDIFrameSource.h"
#include "DwmSharedSurfaceFrameSource.h"
#include "DesktopDuplicationFrameSource.h"
#include "ExclModeHack.h"
#include "Renderer.h"
#include "DeviceResources.h"
#include "GPUTimer.h"
#include "Logger.h"


const UINT WM_DESTORYHOST = RegisterWindowMessage(L"MAGPIE_WM_DESTORYHOST");

static constexpr const wchar_t* HOST_WINDOW_CLASS_NAME = L"Window_Magpie_967EB565-6F73-4E94-AE53-00CC42592A22";
static constexpr const wchar_t* DDF_WINDOW_CLASS_NAME = L"Window_Magpie_C322D752-C866-4630-91F5-32CB242A8930";
static constexpr const wchar_t* HOST_WINDOW_TITLE = L"Magpie_Host";


App::App() {}

App::~App() {
	MagUninitialize();
	winrt::uninit_apartment();
}

bool App::Initialize(HINSTANCE hInst) {
	Logger::Get().Info("正在初始化 App");

	_hInst = hInst;

	// 初始化 COM
	winrt::init_apartment(winrt::apartment_type::multi_threaded);

	_RegisterWndClasses();

	// 供隐藏光标和 MagCallback 抓取模式使用
	if (!MagInitialize()) {
		Logger::Get().Win32Error("MagInitialize 失败");
	}

	Logger::Get().Info("App 初始化成功");
	return true;
}

bool App::Run(
	HWND hwndSrc,
	const std::string& effectsJson,
	UINT captureMode,
	float cursorZoomFactor,
	UINT cursorInterpolationMode,
	int adapterIdx,
	UINT multiMonitorUsage,
	const RECT& cropBorders,
	UINT flags
) {
	_hwndSrc = hwndSrc;
	_cursorZoomFactor = cursorZoomFactor;
	_cursorInterpolationMode = cursorInterpolationMode;
	_adapterIdx = adapterIdx;
	_multiMonitorUsage = multiMonitorUsage;
	_cropBorders = cropBorders;
	_flags = flags;

	Logger::Get().Info(fmt::format("运行时参数：\n\thwndSrc：{}\n\tcaptureMode：{}\n\tadjustCursorSpeed：{}\n\tdisableLowLatency：{}\n\tbreakpointMode：{}\n\tdisableWindowResizing：{}\n\tdisableDirectFlip：{}\n\tconfineCursorIn3DGames：{}\n\tadapterIdx：{}\n\tcropTitleBarOfUWP：{}\n\tmultiMonitorUsage: {}\n\tnoCursor: {}\n\tdisableEffectCache: {}\n\tsimulateExclusiveFullscreen: {}\n\tcursorInterpolationMode: {}\n\tcropLeft: {}\n\tcropTop: {}\n\tcropRight: {}\n\tcropBottom: {}", (void*)hwndSrc, captureMode, IsAdjustCursorSpeed(), IsDisableLowLatency(), IsBreakpointMode(), IsDisableWindowResizing(), IsDisableDirectFlip(), IsConfineCursorIn3DGames(), adapterIdx, IsCropTitleBarOfUWP(), multiMonitorUsage, IsNoCursor(), IsDisableEffectCache(), IsSimulateExclusiveFullscreen(), cursorInterpolationMode, cropBorders.left, cropBorders.top, cropBorders.right, cropBorders.bottom));
	
	SetErrorMsg(ErrorMessages::GENERIC);

	// 模拟独占全屏
	// 必须在主窗口创建前，否则 SHQueryUserNotificationState 可能返回 QUNS_BUSY 而不是 QUNS_RUNNING_D3D_FULL_SCREEN
	ExclModeHack exclMode;

	if (!_CreateHostWnd()) {
		Logger::Get().Critical("创建主窗口失败");
		_OnQuit();
		return false;
	}

	_deviceResources.reset(new DeviceResources());
	if (!_deviceResources->Initialize()) {
		Logger::Get().Critical("初始化 DeviceResources 失败");
		Quit();
		_RunMessageLoop();
		return false;
	}
	
	if (!_InitFrameSource(captureMode)) {
		Logger::Get().Critical("_InitFrameSource 失败");
		Quit();
		_RunMessageLoop();
		return false;
	}

	_renderer.reset(new Renderer());
	if (!_renderer->Initialize(effectsJson)) {
		Logger::Get().Critical("初始化 Renderer 失败");
		Quit();
		_RunMessageLoop();
		return false;
	}

	if (IsDisableDirectFlip() && !IsBreakpointMode()) {
		// 在此处创建的 DDF 窗口不会立刻显示
		if (!_DisableDirectFlip()) {
			Logger::Get().Error("_DisableDirectFlip 失败");
		}
	}

	ShowWindow(_hwndHost, SW_NORMAL);

	_RunMessageLoop();

	return true;
}

void App::_RunMessageLoop() {
	Logger::Get().Info("开始接收窗口消息");

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

winrt::com_ptr<IWICImagingFactory2> App::GetWICImageFactory() {
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

UINT App::RegisterWndProcHandler(std::function<bool(HWND, UINT, WPARAM, LPARAM)> handler) {
	UINT id = _nextWndProcHandlerID++;
	return _wndProcHandlers.emplace(id, handler).second ? id : 0;
}

void App::UnregisterWndProcHandler(UINT id) {
	_wndProcHandlers.erase(id);
}


LRESULT DDFWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_DESTROY) {
		return 0;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

// 注册窗口类
void App::_RegisterWndClasses() const {
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

static BOOL CALLBACK MonitorEnumProc(HMONITOR, HDC, LPRECT monitorRect, LPARAM data) {
	RECT* params = (RECT*)data;

	if (Utils::CheckOverlap(params[0], *monitorRect)) {
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

		if (!Utils::GetWindowFrameRect(hWnd, params[0])) {
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
bool App::_CreateHostWnd() {
	if (FindWindow(HOST_WINDOW_CLASS_NAME, nullptr)) {
		Logger::Get().Error("已存在主窗口");
		return false;
	}

	if (!CalcHostWndRect(_hwndSrc, GetMultiMonitorUsage(), _hostWndRect)) {
		Logger::Get().Error("CalcHostWndRect 失败");
		return false;
	}

	// 主窗口没有覆盖 Virtual Screen 则使用多屏幕模式
	// 启用“在 3D 游戏中限制光标”或断点模式时不使用多屏幕模式
	_isMultiMonitorMode = !IsConfineCursorIn3DGames() && !IsBreakpointMode() && GetMultiMonitorUsage() != 2 &&
		((_hostWndRect.right - _hostWndRect.left) < GetSystemMetrics(SM_CXVIRTUALSCREEN) ||
		(_hostWndRect.bottom - _hostWndRect.top) < GetSystemMetrics(SM_CYVIRTUALSCREEN));

	_hwndHost = CreateWindowEx(
		(IsBreakpointMode() ? 0 : WS_EX_TOPMOST) | WS_EX_NOACTIVATE | WS_EX_LAYERED | WS_EX_TRANSPARENT,
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
	if (!SetLayeredWindowAttributes(_hwndHost, 0, IsDisableDirectFlip() ? 254 : 255, LWA_ALPHA)) {
		Logger::Get().Win32Error("SetLayeredWindowAttributes 失败");
	}

	Logger::Get().Info("已创建主窗口");
	return true;
}

bool App::_InitFrameSource(int captureMode) {
	switch (captureMode) {
	case 0:
		_frameSource.reset(new GraphicsCaptureFrameSource());
		break;
	case 1:
		_frameSource.reset(new DesktopDuplicationFrameSource());
		break;
	case 2:
		_frameSource.reset(new GDIFrameSource());
		break;
	case 3:
		_frameSource.reset(new DwmSharedSurfaceFrameSource());
		break;
	default:
		Logger::Get().Critical("未知的捕获模式");
		return false;
	}

	if (!_frameSource->Initialize()) {
		Logger::Get().Critical("初始化 FrameSource 失败");
		return false;
	}

	const RECT& frameRect = _frameSource->GetSrcFrameRect();
	Logger::Get().Info(fmt::format("源窗口尺寸：{}x{}",
		frameRect.right - frameRect.left, frameRect.bottom - frameRect.top));

	return true;
}

bool App::_DisableDirectFlip() {
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
		const RTL_OSVERSIONINFOW& version = Utils::GetOSVersion();
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

LRESULT App::_HostWndProcStatic(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	return Get()._HostWndProc(hWnd, msg, wParam, lParam);
}


LRESULT App::_HostWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	for (auto& pair : _wndProcHandlers) {
		if (pair.second(hWnd, message, wParam, lParam)) {
			return TRUE;
		}
	}

	if (message == WM_DESTORYHOST) {
		Logger::Get().Info("收到 MAGPIE_WM_DESTORYHOST 消息，即将销毁主窗口");
		Quit();
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
	default:
		break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

void App::_OnQuit() {
	// 释放资源
	_renderer = nullptr;
	_frameSource = nullptr;
	_deviceResources = nullptr;

	_nextWndProcHandlerID = 1;
	_wndProcHandlers.clear();
}

void App::Quit() {
	if (_hwndDDF) {
		DestroyWindow(_hwndDDF);
	}
	if (_hwndHost) {
		DestroyWindow(_hwndHost);
	}
}
