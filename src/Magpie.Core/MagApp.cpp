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
#include "WindowHelper.h"

namespace Magpie::Core {

static constexpr const wchar_t* HOST_WINDOW_CLASS_NAME = L"Window_Magpie_967EB565-6F73-4E94-AE53-00CC42592A22";
static constexpr const wchar_t* DDF_WINDOW_CLASS_NAME = L"Window_Magpie_C322D752-C866-4630-91F5-32CB242A8930";


static LRESULT DDFWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_DESTROY) {
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

			co_await 400ms;
			co_await dispatcher;

			if (app.GetHwndHost()) {
				app.GetCursorManager().Show();
			}
		})();
	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

MagApp::MagApp() :
	_hInst(GetModuleHandle(nullptr)),
	_dispatcher(winrt::DispatcherQueue::GetForCurrentThread())
{
}

MagApp::~MagApp() {}

static bool CheckSrcWindow(HWND hwndSrc, bool isAllowScalingMaximized) {
	if (!WindowHelper::IsValidSrcWindow(hwndSrc)) {
		Logger::Get().Info("禁止缩放系统窗口");
		return false;
	}

	// 不缩放最大化和最小化的窗口
	if (UINT showCmd = Win32Utils::GetWindowShowCmd(hwndSrc); showCmd != SW_NORMAL) {
		if (showCmd != SW_SHOWMAXIMIZED || !isAllowScalingMaximized) {
			Logger::Get().Info(StrUtils::Concat("源窗口已",
				showCmd == SW_SHOWMAXIMIZED ? "最大化" : "最小化"));
			return false;
		}
	}

	// 不缩放过小的窗口
	RECT clientRect{};
	GetClientRect(hwndSrc, &clientRect);
	SIZE clientSize = Win32Utils::GetSizeOfRect(clientRect);
	if (clientSize.cx < 5 || clientSize.cy < 5) {
		Logger::Get().Info("源窗口尺寸过小");
		return false;
	}

#if _DEBUG
	OutputDebugString(fmt::format(L"可执行文件路径：{}\n窗口类：{}\n",
		Win32Utils::GetPathOfWnd(hwndSrc), Win32Utils::GetWndClassName(hwndSrc)).c_str());
#endif // _DEBUG

	return true;
}

bool MagApp::Start(HWND hwndSrc, MagOptions&& options) {
	if (_hwndHost) {
		return false;
	}
	
	if (!CheckSrcWindow(hwndSrc, options.IsAllowScalingMaximized())) {
		return false;
	}

	_hwndSrc = hwndSrc;
	_options = options;

	_RegisterWndClasses();

	if (!_CreateHostWnd()) {
		_hwndSrc = NULL;
		return false;
	}

	_deviceResources = std::make_unique<DeviceResources>();
	if (!_deviceResources->Initialize()) {
		Logger::Get().Error("初始化 DeviceResources 失败");
		Stop();
		return false;
	}

	if (!_InitFrameSource()) {
		Logger::Get().Error("_InitFrameSource 失败");
		Stop();
		return false;
	}

	_renderer = std::make_unique<Renderer>();
	if (!_renderer->Initialize()) {
		Logger::Get().Error("初始化 Renderer 失败");
		Stop();
		return false;
	}

	_cursorManager = std::make_unique<CursorManager>();
	if (!_cursorManager->Initialize()) {
		Logger::Get().Error("初始化 CursorManager 失败");
		Stop();
		return false;
	}

	if (_options.IsDisableDirectFlip() && !_options.IsDebugMode()) {
		// 在此处创建的 DDF 窗口不会立刻显示
		if (!_DisableDirectFlip()) {
			Logger::Get().Error("_DisableDirectFlip 失败");
		}
	}

	_hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);

	assert(_hwndHost); 
	// 缩放窗口可能有 WS_MAXIMIZE 样式，因此使用 SetWindowsPos 而不是 ShowWindow 
	// 以避免 OS 更改窗口尺寸和位置。
	SetWindowPos(
		_hwndHost,
		NULL,
		_hostWndRect.left,
		_hostWndRect.top,
		_hostWndRect.right - _hostWndRect.left,
		_hostWndRect.bottom - _hostWndRect.top,
		SWP_SHOWWINDOW | SWP_NOCOPYBITS | SWP_NOREDRAW
	);

	// 模拟独占全屏
	if (MagApp::Get().GetOptions().IsSimulateExclusiveFullscreen()) {
		// 延迟 1s 以避免干扰游戏的初始化，见 #495
		([](HWND hwndHost)->winrt::fire_and_forget {
			co_await 1s;
			MagApp::Get()._dispatcher.TryEnqueue([hwndHost]() {
				MagApp& app = MagApp::Get();
				// 缩放窗口句柄相同就认为中途没有退出缩放。
				// 实践中很难创建出两个句柄相同的窗口，见 https://stackoverflow.com/a/65617844
				if (app._hwndHost == hwndHost && app._options.IsSimulateExclusiveFullscreen() && !app._exclModeHack) {
					app._exclModeHack = std::make_unique<ExclModeHack>();
				}
			});
		})(_hwndHost);
	};

	return true;
}

winrt::fire_and_forget MagApp::_WaitForSrcMovingOrSizing() {
	HWND hwndSrc = _hwndSrc;
	while (true) {
		if (!IsWindow(hwndSrc) || GetForegroundWindow() != hwndSrc) {
			break;
		} else if (UINT showCmd = Win32Utils::GetWindowShowCmd(hwndSrc); showCmd != SW_NORMAL) {
			if (showCmd != SW_SHOWMAXIMIZED || !MagApp::Get().GetOptions().IsAllowScalingMaximized()) {
				break;
			}
		}

		// 检查源窗口是否正在调整大小或移动
		GUITHREADINFO guiThreadInfo{};
		guiThreadInfo.cbSize = sizeof(GUITHREADINFO);
		if (!GetGUIThreadInfo(GetWindowThreadProcessId(hwndSrc, nullptr), &guiThreadInfo)) {
			Logger::Get().Win32Error("GetGUIThreadInfo 失败");
			break;
		}

		if (guiThreadInfo.flags & GUI_INMOVESIZE) {
			co_await 10ms;
		} else {
			_dispatcher.TryEnqueue([this]() {
				_isWaitingForSrcMovingOrSizing = false;
				Start(_hwndSrc, std::move(_options));
			});
			co_return;
		}
	}

	_dispatcher.TryEnqueue([this]() {
		_isWaitingForSrcMovingOrSizing = false;
	});	
}

void MagApp::Stop(bool isSrcMovingOrSizing) {
	if (_hwndHost) {
		_dispatcher.TryEnqueue([this, isSrcMovingOrSizing]() {
			_isWaitingForSrcMovingOrSizing = isSrcMovingOrSizing;

			DestroyWindow(_hwndHost);

			if(isSrcMovingOrSizing) {
				// 源窗口的大小或位置不再改变时重新缩放
				_WaitForSrcMovingOrSizing();
			}
		});
	}
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

bool MagApp::MessageLoop() {
	if (!_hwndHost) {
		return true;
	}

	while (true) {
		MSG msg;
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				Stop();
				return false;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (!_hwndHost) {
			if (_isWaitingForSrcMovingOrSizing) {
				// 防止 CPU 占用过高
				WaitMessage();
				continue;
			} else {
				return true;
			}
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

	return true;
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
			Logger::Get().Win32Error("注册缩放窗口类失败");
		}

		wcex.lpfnWndProc = DDFWndProc;
		wcex.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
		wcex.lpszClassName = DDF_WINDOW_CLASS_NAME;

		if (!RegisterClassEx(&wcex)) {
			Logger::Get().Win32Error("注册 DDF 窗口类失败");
		}
	}
}

// 返回缩放窗口跨越的屏幕数量，失败返回 0
static uint32_t CalcHostWndRect(HWND hWnd, MultiMonitorUsage multiMonitorUsage, RECT& result) {
	switch (multiMonitorUsage) {
	case MultiMonitorUsage::Closest:
	{
		// 使用距离源窗口最近的显示器
		HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		if (!hMonitor) {
			Logger::Get().Win32Error("MonitorFromWindow 失败");
			return 0;
		}

		MONITORINFO mi{};
		mi.cbSize = sizeof(mi);
		if (!GetMonitorInfo(hMonitor, &mi)) {
			Logger::Get().Win32Error("GetMonitorInfo 失败");
			return 0;
		}
		result = mi.rcMonitor;

		return 1;
	}
	case MultiMonitorUsage::Intersected:
	{
		// 使用源窗口跨越的所有显示器

		// [0] 存储源窗口坐标，[1] 存储计算结果
		struct MonitorEnumParam {
			RECT srcRect;
			RECT destRect;
			uint32_t monitorCount;
		} param{};

		if (!Win32Utils::GetWindowFrameRect(hWnd, param.srcRect)) {
			Logger::Get().Error("GetWindowFrameRect 失败");
			return 0;
		}

		MONITORENUMPROC monitorEnumProc = [](HMONITOR, HDC, LPRECT monitorRect, LPARAM data) {
			MonitorEnumParam* param = (MonitorEnumParam*)data;

			if (Win32Utils::CheckOverlap(param->srcRect, *monitorRect)) {
				UnionRect(&param->destRect, monitorRect, &param->destRect);
				++param->monitorCount;
			}

			return TRUE;
		};

		if (!EnumDisplayMonitors(NULL, NULL, monitorEnumProc, (LPARAM)&param)) {
			Logger::Get().Win32Error("EnumDisplayMonitors 失败");
			return 0;
		}

		result = param.destRect;
		if (result.right - result.left <= 0 || result.bottom - result.top <= 0) {
			Logger::Get().Error("计算缩放窗口坐标失败");
			return 0;
		}

		return param.monitorCount;
	}
	case MultiMonitorUsage::All:
	{
		// 使用所有显示器（Virtual Screen）
		int vsWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
		int vsHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
		int vsX = GetSystemMetrics(SM_XVIRTUALSCREEN);
		int vsY = GetSystemMetrics(SM_YVIRTUALSCREEN);
		result = { vsX, vsY, vsX + vsWidth, vsY + vsHeight };

		return GetSystemMetrics(SM_CMONITORS);
	}
	default:
		return 0;
	}
}

// 创建缩放窗口
bool MagApp::_CreateHostWnd() {
	if (FindWindow(HOST_WINDOW_CLASS_NAME, nullptr)) {
		Logger::Get().Error("已存在缩放窗口");
		return false;
	}

	const uint32_t monitors = CalcHostWndRect(_hwndSrc, _options.multiMonitorUsage, _hostWndRect);
	if (monitors == 0) {
		Logger::Get().Error("CalcHostWndRect 失败");
		return false;
	}

	if (!_options.IsAllowScalingMaximized()) {
		// 源窗口和缩放窗口重合则不缩放，此时源窗口可能是无边框全屏窗口
		RECT srcRect;
		if (!Win32Utils::GetWindowFrameRect(_hwndSrc, srcRect)) {
			Win32Utils::GetClientScreenRect(_hwndSrc, srcRect);
		}

		if (srcRect == _hostWndRect) {
			Logger::Get().Info("源窗口已全屏");
			return false;
		}
	}
	
	// WS_EX_NOREDIRECTIONBITMAP 可以避免 WS_EX_LAYERED 导致的额外内存开销。
	// WS_MAXIMIZE 使 Wallpaper Engine 在缩放时暂停动态壁纸 #502，这个 hack 不支持
	// 跨越多个屏幕的情况。
	_hwndHost = CreateWindowEx(
		(_options.IsDebugMode() ? 0 : WS_EX_TOPMOST) | WS_EX_NOACTIVATE
			| WS_EX_LAYERED | WS_EX_NOREDIRECTIONBITMAP | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
		HOST_WINDOW_CLASS_NAME,
		nullptr,	// 标题为空，否则会被添加新配置页面列为候选窗口
		WS_POPUP | (monitors == 1 ? WS_MAXIMIZE : 0),
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
		Logger::Get().Win32Error("创建缩放窗口失败");
		return false;
	}

	Logger::Get().Info(fmt::format("缩放窗口尺寸：{}x{}",
		_hostWndRect.right - _hostWndRect.left, _hostWndRect.bottom - _hostWndRect.top));

	// 设置窗口不透明
	// 不完全透明时可关闭 DirectFlip
	if (!SetLayeredWindowAttributes(_hwndHost, 0, _options.IsDisableDirectFlip() ? 254 : 255, LWA_ALPHA)) {
		Logger::Get().Win32Error("SetLayeredWindowAttributes 失败");
	}

	return true;
}

bool MagApp::_InitFrameSource() {
	switch (_options.captureMethod) {
	case CaptureMethod::GraphicsCapture:
		_frameSource = std::make_unique<GraphicsCaptureFrameSource>();
		break;
	case CaptureMethod::DesktopDuplication:
		_frameSource = std::make_unique<DesktopDuplicationFrameSource>();
		break;
	case CaptureMethod::GDI:
		_frameSource = std::make_unique<GDIFrameSource>();
		break;
	case CaptureMethod::DwmSharedSurface:
		_frameSource = std::make_unique<DwmSharedSurfaceFrameSource>();
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
		if (Win32Utils::GetOSVersion().Is20H1OrNewer()) {
			// 使 DDF 窗口无法被捕获到
			if (!SetWindowDisplayAffinity(_hwndDDF, WDA_EXCLUDEFROMCAPTURE)) {
				Logger::Get().Win32Error("SetWindowDisplayAffinity 失败");
			}
		}
	}

	return true;
}

LRESULT MagApp::_HostWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	// 以反向调用回调
	for (auto it = _wndProcHandlers.rbegin(); it != _wndProcHandlers.rend(); ++it) {
		const auto& result = it->first(hWnd, msg, wParam, lParam);
		if (result.has_value()) {
			return *result;
		}
	}

	switch (msg) {
	case WM_DESTROY:
	{
		_OnQuit();

		if (_hwndDDF) {
			DestroyWindow(_hwndDDF);
			_hwndDDF = NULL;
		}

		_hwndHost = NULL;
		return 0;
	}
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

void MagApp::_OnQuit() {
	if (_hKeyboardHook) {
		UnhookWindowsHookEx(_hKeyboardHook);
		_hKeyboardHook = NULL;
	}

	// 释放资源
	_exclModeHack.reset();
	_cursorManager.reset();
	_renderer.reset();
	_frameSource.reset();
	_deviceResources.reset();

	_nextWndProcHandlerID = 1;
	_wndProcHandlers.clear();
}

}
