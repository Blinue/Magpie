#include "pch.h"
#include "App.h"
#include "Utils.h"
#include "GraphicsCaptureFrameSource.h"
#include "GDIFrameSource.h"
#include "DwmSharedSurfaceFrameSource.h"
#include "DesktopDuplicationFrameSource.h"


extern std::shared_ptr<spdlog::logger> logger;

const UINT WM_DESTORYHOST = RegisterWindowMessage(L"MAGPIE_WM_DESTORYHOST");

static constexpr const wchar_t* HOST_WINDOW_CLASS_NAME = L"Window_Magpie_967EB565-6F73-4E94-AE53-00CC42592A22";
static constexpr const wchar_t* DDF_WINDOW_CLASS_NAME = L"Window_Magpie_C322D752-C866-4630-91F5-32CB242A8930";
static constexpr const wchar_t* HOST_WINDOW_TITLE = L"Magpie_Host";


App::~App() {
	MagUninitialize();
	winrt::uninit_apartment();
}

bool App::Initialize(HINSTANCE hInst) {
	SPDLOG_LOGGER_INFO(logger, "正在初始化 App");

	_hInst = hInst;

	// 初始化 COM
	winrt::init_apartment(winrt::apartment_type::multi_threaded);

	_RegisterWndClasses();

	// 供隐藏光标和 MagCallback 抓取模式使用
	if (!MagInitialize()) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("MagInitialize 失败"));
	}

	SPDLOG_LOGGER_INFO(logger, "App 初始化成功");
	return true;
}

static BOOL CALLBACK EnumChildProc(
	_In_ HWND   hwnd,
	_In_ LPARAM lParam
) {
	std::wstring className(256, 0);
	int num = GetClassName(hwnd, &className[0], (int)className.size());
	if (num == 0) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetClassName 失败"));
		return TRUE;
	}
	className.resize(num);

	if (className == L"ApplicationFrameInputSinkWindow") {
		((std::vector<HWND>*)lParam)->push_back(hwnd);
	}

	return TRUE;
}

HWND FindClientWindow(HWND hwndSrc) {
	std::wstring className(256, 0);
	int num = GetClassName(hwndSrc, &className[0], (int)className.size());
	if (num > 0) {
		className.resize(num);
		if (className == L"ApplicationFrameWindow" || className == L"Windows.UI.Core.CoreWindow") {
			// "Modern App"
			std::vector<HWND> childWindows;
			// 查找所有窗口类名为 ApplicationFrameInputSinkWindow 的子窗口
			// 该子窗口一般为客户区
			EnumChildWindows(hwndSrc, EnumChildProc, (LPARAM)&childWindows);

			if (!childWindows.empty()) {
				// 如果有多个匹配的子窗口，取最大的（一般不会出现）
				int maxSize = 0, maxIdx = 0;
				for (int i = 0; i < childWindows.size(); ++i) {
					RECT rect;
					if (!GetClientRect(childWindows[i], &rect)) {
						continue;
					}

					int size = rect.right - rect.left + rect.bottom - rect.top;
					if (size > maxSize) {
						maxSize = size;
						maxIdx = i;
					}
				}

				return childWindows[maxIdx];
			}
		}
	} else {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetClassName 失败"));
	}

	return hwndSrc;
}

bool App::Run(
	HWND hwndSrc,
	const std::string& effectsJson,
	UINT captureMode,
	int frameRate,
	float cursorZoomFactor,
	UINT cursorInterpolationMode,
	UINT adapterIdx,
	UINT multiMonitorUsage,
	UINT flags
) {
	_hwndSrc = hwndSrc;
	_captureMode = captureMode;
	_frameRate = frameRate;
	_cursorZoomFactor = cursorZoomFactor;
	_cursorInterpolationMode = cursorInterpolationMode;
	_adapterIdx = adapterIdx;
	_multiMonitorUsage = multiMonitorUsage;
	_flags = flags;

	SPDLOG_LOGGER_INFO(logger, fmt::format("运行时参数：\n\thwndSrc：{}\n\tcaptureMode：{}\n\tadjustCursorSpeed：{}\n\tshowFPS：{}\n\tframeRate：{}\n\tdisableLowLatency：{}\n\tbreakpointMode：{}\n\tdisableWindowResizing：{}\n\tdisableDirectFlip：{}\n\tConfineCursorIn3DGames：{}\n\tadapterIdx：{}\n\tCropTitleBarOfUWP：{}\n\tmultiMonitorMode: {}", (void*)hwndSrc, captureMode, IsAdjustCursorSpeed(), IsShowFPS(), frameRate, IsDisableLowLatency(), IsBreakpointMode(), IsDisableWindowResizing(), IsDisableDirectFlip(), IsConfineCursorIn3DGames(), adapterIdx, IsCropTitleBarOfUWP(), multiMonitorUsage));

	// 每次进入全屏都要重置
	_nextTimerId = 1;
	
	SetErrorMsg(ErrorMessages::GENERIC);

	// 禁用窗口大小调整
	bool windowResizingDisabled = false;
	if (IsDisableWindowResizing()) {
		LONG_PTR style = GetWindowLongPtr(hwndSrc, GWL_STYLE);
		if (style & WS_THICKFRAME) {
			if (SetWindowLongPtr(hwndSrc, GWL_STYLE, style ^ WS_THICKFRAME)) {
				// 不重绘边框，以防某些窗口状态不正确
				// if (!SetWindowPos(hwndSrc, 0, 0, 0, 0, 0,
				//	SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED)) {
				//	SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("SetWindowPos 失败"));
				// }

				SPDLOG_LOGGER_INFO(logger, "已禁用窗口大小调整");
				windowResizingDisabled = true;
			} else {
				SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("禁用窗口大小调整失败"));
			}
		}
	}

	_hwndSrcClient = IsCropTitleBarOfUWP() ? FindClientWindow(hwndSrc) : hwndSrc;

	if (!_CreateHostWnd()) {
		SPDLOG_LOGGER_CRITICAL(logger, "创建主窗口失败");
		return false;
	}

	_renderer.reset(new Renderer());
	if (!_renderer->Initialize()) {
		SPDLOG_LOGGER_CRITICAL(logger, "初始化 Renderer 失败，正在清理");
		Close();
		_Run();
		return false;
	}
	
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
		SPDLOG_LOGGER_CRITICAL(logger, "未知的捕获模式，即将退出");
		Close();
		_Run();
		return false;
	}
	
	if (!_frameSource->Initialize()) {
		SPDLOG_LOGGER_CRITICAL(logger, "初始化 FrameSource 失败，即将退出");
		Close();
		_Run();
		return false;
	}

	// FrameSource 初始化完成后计算窗口边框，因为初始化过程中可能改变窗口位置
	if (!Utils::GetClientScreenRect(_hwndSrcClient, _srcClientRect)) {
		SPDLOG_LOGGER_ERROR(logger, "获取源窗口客户区失败");
	}

	SPDLOG_LOGGER_INFO(logger, fmt::format("源窗口客户区尺寸：{}x{}",
		_srcClientRect.right - _srcClientRect.left, _srcClientRect.bottom - _srcClientRect.top));

	if (!_renderer->InitializeEffectsAndCursor(effectsJson)) {
		SPDLOG_LOGGER_CRITICAL(logger, "初始化效果失败，即将退出");
		Close();
		_Run();
		return false;
	}

	// 禁用窗口圆角
	bool roundCornerDisabled = false;
	if (_frameSource->HasRoundCornerInWin11()) {
		const auto& version = Utils::GetOSVersion();
		bool isWin11 = Utils::CompareVersion(
			version.dwMajorVersion, version.dwMinorVersion,
			version.dwBuildNumber, 10, 0, 22000) >= 0;

		if (isWin11) {
			INT attr = DWMWCP_DONOTROUND;
			HRESULT hr = DwmSetWindowAttribute(hwndSrc, DWMWA_WINDOW_CORNER_PREFERENCE, &attr, sizeof(attr));
			if (FAILED(hr)) {
				SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("禁用窗口圆角失败", hr));
			} else {
				SPDLOG_LOGGER_INFO(logger, "已禁用窗口圆角");
				roundCornerDisabled = true;
			}
		}
	}

	if (!IsBreakpointMode() && IsDisableDirectFlip()) {
		if (!_DisableDirectFlip()) {
			SPDLOG_LOGGER_ERROR(logger, "_DisableDirectFlip 失败");
		}
	}

	_Run();

	// 还原窗口圆角
	if (roundCornerDisabled) {
		INT attr = DWMWCP_DEFAULT;
		HRESULT hr = DwmSetWindowAttribute(hwndSrc, DWMWA_WINDOW_CORNER_PREFERENCE, &attr, sizeof(attr));
		if (FAILED(hr)) {
			SPDLOG_LOGGER_INFO(logger, MakeComErrorMsg("取消禁用窗口圆角失败", hr));
		} else {
			SPDLOG_LOGGER_INFO(logger, "已取消禁用窗口圆角");
		}
	}

	// 还原窗口大小调整
	if (windowResizingDisabled) {
		LONG_PTR style = GetWindowLongPtr(hwndSrc, GWL_STYLE);
		if (!(style & WS_THICKFRAME)) {
			if (SetWindowLongPtr(hwndSrc, GWL_STYLE, style | WS_THICKFRAME)) {
				if (!SetWindowPos(hwndSrc, 0, 0, 0, 0, 0,
					SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED)) {
					SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("SetWindowPos 失败"));
				}

				SPDLOG_LOGGER_INFO(logger, "已取消禁用窗口大小调整");
			} else {
				SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("取消禁用窗口大小调整失败"));
			}
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
	wcex.lpszClassName = HOST_WINDOW_CLASS_NAME;

	if (!RegisterClassEx(&wcex)) {
		// 忽略此错误，因为可能是重复注册产生的错误
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("注册主窗口类失败"));
	} else {
		SPDLOG_LOGGER_INFO(logger, "已注册主窗口类");
	}

	wcex.lpfnWndProc = DDFWndProc;
	wcex.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
	wcex.lpszClassName = DDF_WINDOW_CLASS_NAME;

	if (!RegisterClassEx(&wcex)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("注册 DDF 窗口类失败"));
	} else {
		SPDLOG_LOGGER_INFO(logger, "已注册 DDF 窗口类");
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
			SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("MonitorFromWindow 失败"));
			return false;
		}

		MONITORINFO mi{};
		mi.cbSize = sizeof(mi);
		if (!GetMonitorInfo(hMonitor, &mi)) {
			SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetMonitorInfo 失败"));
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

		HRESULT hr = DwmGetWindowAttribute(hWnd,
			DWMWA_EXTENDED_FRAME_BOUNDS, &params[0], sizeof(RECT));
		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("DwmGetWindowAttribute 失败", hr));
			return false;
		}
		
		if (!EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)&params)) {
			SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("EnumDisplayMonitors 失败"));
			return false;
		}
		
		result = params[1];
		if (result.right - result.left <= 0 || result.bottom - result.top <= 0) {
			SPDLOG_LOGGER_ERROR(logger, "计算主窗口坐标失败");
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
		SPDLOG_LOGGER_CRITICAL(logger, "已存在主窗口");
		return false;
	}

	if (!CalcHostWndRect(_hwndSrc, GetMultiMonitorUsage(), _hostWndRect)) {
		SPDLOG_LOGGER_ERROR(logger, "CalcHostWndRect 失败");
		return false;
	}

	// 主窗口没有覆盖 Virtual Screen 则使用多屏幕模式
	_isMultiMonitorMode = GetMultiMonitorUsage() != 2 && !IsBreakpointMode() &&
		((_hostWndRect.right - _hostWndRect.left) < GetSystemMetrics(SM_CXVIRTUALSCREEN) ||
		(_hostWndRect.bottom - _hostWndRect.top) < GetSystemMetrics(SM_CYVIRTUALSCREEN));

	_hwndHost = CreateWindowEx(
		(IsBreakpointMode() ? 0 : WS_EX_TOPMOST) | WS_EX_NOACTIVATE | WS_EX_LAYERED | WS_EX_TRANSPARENT,
		HOST_WINDOW_CLASS_NAME,
		HOST_WINDOW_TITLE,
		WS_CLIPCHILDREN | WS_POPUP | WS_VISIBLE,
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
		SPDLOG_LOGGER_CRITICAL(logger, MakeWin32ErrorMsg("创建主窗口失败"));
		return false;
	}

	SPDLOG_LOGGER_INFO(logger, fmt::format("主窗口尺寸：{}x{}",
		_hostWndRect.right - _hostWndRect.left, _hostWndRect.bottom - _hostWndRect.top));

	// 设置窗口不透明
	// 不完全透明时可关闭 DirectFlip
	if (!SetLayeredWindowAttributes(_hwndHost, 0, IsDisableDirectFlip() ? 254 : 255, LWA_ALPHA)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("SetLayeredWindowAttributes 失败"));
	}

	if (!ShowWindow(_hwndHost, SW_NORMAL)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("ShowWindow 失败"));
	}

	SPDLOG_LOGGER_INFO(logger, "已创建主窗口");
	return true;
}

bool App::_DisableDirectFlip() {
	// 没有显式关闭 DirectFlip 的方法
	// 将全屏窗口设为稍微透明，以灰色全屏窗口为背景
	_hwndDDF = CreateWindowEx(
		WS_EX_NOACTIVATE | WS_EX_LAYERED | WS_EX_TRANSPARENT,
		DDF_WINDOW_CLASS_NAME,
		NULL,
		WS_CLIPCHILDREN | WS_POPUP | WS_VISIBLE,
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
		SPDLOG_LOGGER_CRITICAL(logger, MakeWin32ErrorMsg("创建 DDF 窗口失败"));
		return false;
	}

	if (!SetWindowPos(_hwndDDF, _hwndHost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOREDRAW)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("SetWindowPos 失败"));
	}

	// 设置窗口不透明
	if (!SetLayeredWindowAttributes(_hwndDDF, 0, 255, LWA_ALPHA)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("SetLayeredWindowAttributes 失败"));
	}

	if (!ShowWindow(_hwndDDF, SW_NORMAL)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("ShowWindow 失败"));
	}

	SPDLOG_LOGGER_INFO(logger, "已创建 DDF 主窗口");
	return true;
}

LRESULT App::_HostWndProcStatic(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	return GetInstance()._HostWndProc(hWnd, msg, wParam, lParam);
}


LRESULT App::_HostWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	if (message == WM_DESTORYHOST) {
		SPDLOG_LOGGER_INFO(logger, "收到 MAGPIE_WM_DESTORYHOST 消息，即将销毁主窗口");
		Close();
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

void App::Close() {
	if (_hwndDDF) {
		DestroyWindow(_hwndDDF);
	}
	if (_hwndHost) {
		DestroyWindow(_hwndHost);
	}
}
