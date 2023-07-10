#include "pch.h"
#include "ScalingWindow.h"
#include "CommonSharedConstants.h"
#include "Logger.h"
#include "Renderer.h"
#include "Win32Utils.h"
#include "WindowHelper.h"
#include "CursorManager.h"
#include <timeapi.h>

namespace Magpie::Core {

ScalingWindow::ScalingWindow() noexcept {}

ScalingWindow::~ScalingWindow() noexcept {}

static BOOL CALLBACK MonitorEnumProc(HMONITOR, HDC, LPRECT monitorRect, LPARAM data) noexcept {
	RECT* params = (RECT*)data;

	if (Win32Utils::CheckOverlap(params[0], *monitorRect)) {
		UnionRect(&params[1], monitorRect, &params[1]);
	}

	return TRUE;
}

static bool CalcRect(HWND hWnd, MultiMonitorUsage multiMonitorUsage, RECT& result) noexcept {
	switch (multiMonitorUsage) {
	case MultiMonitorUsage::Closest:
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

		HRESULT hr = DwmGetWindowAttribute(hWnd,
			DWMWA_EXTENDED_FRAME_BOUNDS, &params[0], sizeof(params[0]));
		if (FAILED(hr)) {
			Logger::Get().ComError("DwmGetWindowAttribute 失败", hr);
			return false;
		}

		if (!EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)&params)) {
			Logger::Get().Win32Error("EnumDisplayMonitors 失败");
			return false;
		}

		result = params[1];
		if (result.right - result.left <= 0 || result.bottom - result.top <= 0) {
			Logger::Get().Error("计算缩放窗口坐标失败");
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

bool ScalingWindow::Create(HINSTANCE hInstance, HWND hwndSrc, ScalingOptions&& options) noexcept {
	if (_hWnd) {
		return false;
	}

	_hwndSrc = hwndSrc;
	// 缩放结束后才失效
	_options = std::move(options);

	if (FindWindow(CommonSharedConstants::SCALING_WINDOW_CLASS_NAME, nullptr)) {
		Logger::Get().Error("已存在缩放窗口");
		return false;
	}

	// 提高时钟精度，默认为 15.6ms
	timeBeginPeriod(1);

	if (!CalcRect(_hwndSrc, _options.multiMonitorUsage, _wndRect)) {
		Logger::Get().Error("CalcRect 失败");
		return false;
	}

	if (!_options.IsAllowScalingMaximized()) {
		// 源窗口和缩放窗口重合则不缩放，此时源窗口可能是无边框全屏窗口
		RECT srcRect{};
		HRESULT hr = DwmGetWindowAttribute(_hwndSrc,
			DWMWA_EXTENDED_FRAME_BOUNDS, &srcRect, sizeof(srcRect));
		if (FAILED(hr)) {
			Win32Utils::GetClientScreenRect(_hwndSrc, srcRect);
		}

		if (srcRect == _wndRect) {
			Logger::Get().Info("源窗口已全屏");
			return false;
		}
	}

	static const int _ = [](HINSTANCE hInstance) {
		WNDCLASSEXW wcex{};
		wcex.cbSize = sizeof(wcex);
		wcex.lpfnWndProc = _WndProc;
		wcex.hInstance = hInstance;
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.lpszClassName = CommonSharedConstants::SCALING_WINDOW_CLASS_NAME;
		RegisterClassEx(&wcex);

		return 0;
	}(hInstance);

	CreateWindowEx(
		(_options.IsDebugMode() ? 0 : WS_EX_TOPMOST) | WS_EX_NOACTIVATE
		| WS_EX_LAYERED | WS_EX_NOREDIRECTIONBITMAP | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
		CommonSharedConstants::SCALING_WINDOW_CLASS_NAME,
		L"Magpie",
		WS_POPUP,
		_wndRect.left,
		_wndRect.top,
		_wndRect.right - _wndRect.left,
		_wndRect.bottom - _wndRect.top,
		NULL,
		NULL,
		hInstance,
		this
	);

	if (!_hWnd) {
		return false;
	}

	// 设置窗口不透明
	// 不完全透明时可关闭 DirectFlip
	if (!SetLayeredWindowAttributes(_hWnd, 0, 255, LWA_ALPHA)) {
		Logger::Get().Win32Error("SetLayeredWindowAttributes 失败");
	}

	if (!GetWindowRect(hwndSrc, &_srcWndRect)) {
		Logger::Get().Win32Error("GetWindowRect 失败");
		Destroy();
		return false;
	}

	_renderer = std::make_unique<class Renderer>();
	if (!_renderer->Initialize()) {
		Logger::Get().Error("初始化 Renderer 失败");
		Destroy();
		return false;
	}

	_cursorManager = std::make_unique<class CursorManager>();
	if (!_cursorManager->Initialize()) {
		Logger::Get().Error("初始化 CursorManager 失败");
		Destroy();
		return false;
	}

	ShowWindow(_hWnd, SW_SHOWMAXIMIZED);

	return true;
}

void ScalingWindow::Render() noexcept {
	int srcState = _CheckSrcState();
	if (srcState != 0) {
		Logger::Get().Info("源窗口状态改变，退出全屏");
		Destroy();
		//MagApp::Get().Stop(srcState == 2);
		return;
	}

	_cursorManager->Update();
	_renderer->Render();
}

void ScalingWindow::ToggleOverlay() noexcept {
	_renderer->ToggleOverlay();
}

LRESULT ScalingWindow::_MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
	switch (msg) {
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	{
		// 在以下情况下会收到光标消息：
		// 1、未捕获光标且缩放后的位置未被遮挡而缩放前的位置被遮挡
		// 2、光标位于叠加层上
		const HWND hwndForground = GetForegroundWindow();
		if (hwndForground != _hwndSrc) {
			if (!Win32Utils::SetForegroundWindow(_hwndSrc)) {
				// 设置前台窗口失败，可能是因为前台窗口是开始菜单
				if (WindowHelper::IsStartMenu(hwndForground)) {
					using namespace std::chrono;

					// 限制触发频率
					static steady_clock::time_point prevTimePoint{};
					auto now = steady_clock::now();
					if (duration_cast<milliseconds>(now - prevTimePoint).count() >= 1000) {
						prevTimePoint = now;

						// 模拟按键关闭开始菜单
						INPUT inputs[4]{};
						inputs[0].type = INPUT_KEYBOARD;
						inputs[0].ki.wVk = VK_LWIN;
						inputs[1].type = INPUT_KEYBOARD;
						inputs[1].ki.wVk = VK_LWIN;
						inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
						SendInput((UINT)std::size(inputs), inputs, sizeof(INPUT));

						// 等待系统处理
						Sleep(1);
					}

					SetForegroundWindow(_hwndSrc);
				}
			}
		}

		break;
	}
	case WM_DESTROY:
	{
		_cursorManager.reset();
		_renderer.reset();
		_options = {};
		_hwndSrc = NULL;
		_srcWndRect = {};

		// 还原时钟精度
		timeEndPeriod(1);
		break;
	}
	}
	return base_type::_MessageHandler(msg, wParam, lParam);
}

// 0 -> 可继续缩放
// 1 -> 前台窗口改变或源窗口最大化（如果不允许缩放最大化的窗口）/最小化
// 2 -> 源窗口大小或位置改变或最大化（如果允许缩放最大化的窗口）
int ScalingWindow::_CheckSrcState() const noexcept {
	if (!_options.IsDebugMode()) {
		HWND hwndForeground = GetForegroundWindow();
		// 在 3D 游戏模式下打开游戏内叠加层则全屏窗口可以接收焦点
		if (!_options.Is3DGameMode() /*|| !IsUIVisiable()*/|| hwndForeground != _hWnd) {
			if (hwndForeground && hwndForeground != _hwndSrc && !_CheckForeground(hwndForeground)) {
				Logger::Get().Info("前台窗口已改变");
				return 1;
			}
		}
	}

	UINT showCmd = Win32Utils::GetWindowShowCmd(_hwndSrc);
	if (showCmd != SW_NORMAL && (showCmd != SW_SHOWMAXIMIZED || !_options.IsAllowScalingMaximized())) {
		Logger::Get().Info("源窗口显示状态改变");
		return 1;
	}

	RECT rect;
	if (!GetWindowRect(_hwndSrc, &rect)) {
		Logger::Get().Error("GetWindowRect 失败");
		return 1;
	}

	if (_srcWndRect != rect) {
		Logger::Get().Info("源窗口位置或大小改变");
		return 2;
	}

	return 0;
}

bool ScalingWindow::_CheckForeground(HWND hwndForeground) const noexcept {
	std::wstring className = Win32Utils::GetWndClassName(hwndForeground);

	if (!WindowHelper::IsValidSrcWindow(hwndForeground)) {
		return true;
	}

	RECT rectForground{};
	HRESULT hr = DwmGetWindowAttribute(hwndForeground,
		DWMWA_EXTENDED_FRAME_BOUNDS, &rectForground, sizeof(rectForground));
	if (FAILED(hr)) {
		Logger::Get().ComError("DwmGetWindowAttribute 失败", hr);
		return false;
	}

	RECT scalingWndRect;
	GetWindowRect(_hWnd, &scalingWndRect);
	IntersectRect(&rectForground, &scalingWndRect, &rectForground);

	// 允许稍微重叠，否则前台窗口最大化时会意外退出
	return rectForground.right - rectForground.left < 10 || rectForground.right - rectForground.top < 10;
}

}
