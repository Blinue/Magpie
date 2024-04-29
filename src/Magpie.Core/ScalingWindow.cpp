#include "pch.h"
#include "ScalingWindow.h"
#include "CommonSharedConstants.h"
#include "Logger.h"
#include "Renderer.h"
#include "Win32Utils.h"
#include "WindowHelper.h"
#include "CursorManager.h"
#include <timeapi.h>
#include "FrameSourceBase.h"
#include "ExclModeHelper.h"
#include "StrUtils.h"
#include "Utils.h"

namespace Magpie::Core {

static UINT WM_MAGPIE_SCALINGCHANGED;
// 用于和 TouchHelper 交互
static UINT WM_MAGPIE_TOUCHHELPER;

static void InitMessage() noexcept {
	static Utils::Ignore _ = []() {
		WM_MAGPIE_SCALINGCHANGED =
			RegisterWindowMessage(CommonSharedConstants::WM_MAGPIE_SCALINGCHANGED);
		WM_MAGPIE_TOUCHHELPER =
			RegisterWindowMessage(CommonSharedConstants::WM_MAGPIE_TOUCHHELPER);

		return Utils::Ignore();
	}();
}


ScalingWindow::ScalingWindow() noexcept {}

ScalingWindow::~ScalingWindow() noexcept {}

// 返回缩放窗口跨越的屏幕数量，失败返回 0
static uint32_t CalcWndRect(HWND hWnd, MultiMonitorUsage multiMonitorUsage, RECT& result) {
	switch (multiMonitorUsage) {
	case MultiMonitorUsage::Closest:
	{
		// 使用距离源窗口最近的显示器
		HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		if (!hMonitor) {
			Logger::Get().Win32Error("MonitorFromWindow 失败");
			return 0;
		}

		MONITORINFO mi{ .cbSize = sizeof(mi) };
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

		if (Win32Utils::GetWindowShowCmd(hWnd) == SW_SHOWMAXIMIZED) {
			// 最大化的窗口不能跨越屏幕
			HMONITOR hMon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
			MONITORINFO mi{ .cbSize = sizeof(mi) };
			if (!GetMonitorInfo(hMon, &mi)) {
				Logger::Get().Win32Error("GetMonitorInfo 失败");
				return 0;
			}

			result = mi.rcMonitor;
			return 1;
		} else {
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

bool ScalingWindow::Create(
	const winrt::DispatcherQueue& dispatcher,
	HWND hwndSrc,
	ScalingOptions&& options
) noexcept {
	if (_hWnd) {
		return false;
	}

	InitMessage();

#if _DEBUG
	OutputDebugString(fmt::format(L"可执行文件路径: {}\n窗口类: {}\n",
		Win32Utils::GetPathOfWnd(hwndSrc), Win32Utils::GetWndClassName(hwndSrc)).c_str());
#endif

	_hwndSrc = hwndSrc;
	// 缩放结束后才失效
	_options = std::move(options);
	_dispatcher = dispatcher;

	_isSrcRepositioning = false;

	if (FindWindow(CommonSharedConstants::SCALING_WINDOW_CLASS_NAME, nullptr)) {
		Logger::Get().Error("已存在缩放窗口");
		return false;
	}

	// 记录缩放选项
	_options.Log();

	// 提高时钟精度，默认为 15.6ms
	timeBeginPeriod(1);

	const uint32_t monitors = CalcWndRect(_hwndSrc, _options.multiMonitorUsage, _wndRect);
	if (monitors == 0) {
		Logger::Get().Error("CalcWndRect 失败");
		return false;
	}

	Logger::Get().Info(fmt::format("缩放窗口边界: {},{},{},{}",
		_wndRect.left, _wndRect.top, _wndRect.right, _wndRect.bottom));
	
	if (!_options.IsAllowScalingMaximized()) {
		if (Win32Utils::GetWindowShowCmd(_hwndSrc) == SW_SHOWMAXIMIZED) {
			Logger::Get().Info("源窗口已最大化");
			return false;
		}

		// 源窗口和缩放窗口重合则不缩放，此时源窗口可能是无边框全屏窗口
		RECT srcRect;
		if (!Win32Utils::GetWindowFrameRect(_hwndSrc, srcRect)) {
			Logger::Get().Error("GetWindowFrameRect 失败");
			return false;
		}

		if (srcRect == _wndRect) {
			Logger::Get().Info("源窗口已全屏");
			return false;
		}
	}

	const HINSTANCE hInstance = wil::GetModuleInstanceHandle();

	static Utils::Ignore _ = [](HINSTANCE hInstance) {
		WNDCLASSEXW wcex{
			.cbSize = sizeof(wcex),
			.lpfnWndProc = _WndProc,
			.hInstance = hInstance,
			.hCursor = LoadCursor(nullptr, IDC_ARROW),
			.lpszClassName = CommonSharedConstants::SCALING_WINDOW_CLASS_NAME
		};
		RegisterClassEx(&wcex);

		return Utils::Ignore();
	}(hInstance);

	CreateWindowEx(
		(_options.IsDebugMode() ? 0 : WS_EX_TOPMOST | WS_EX_TRANSPARENT) | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE
		 | WS_EX_NOREDIRECTIONBITMAP,
		CommonSharedConstants::SCALING_WINDOW_CLASS_NAME,
		L"Magpie",
		WS_POPUP | (monitors == 1 ? WS_MAXIMIZE : 0),
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
	if (!SetLayeredWindowAttributes(_hWnd, 0, _options.IsDirectFlipDisabled() ? 254 : 255, LWA_ALPHA)) {
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

	if (_options.IsDirectFlipDisabled() && !_options.IsDebugMode()) {
		// 在此处创建的 DDF 窗口不会立刻显示
		if (!_DisableDirectFlip(hInstance)) {
			Logger::Get().Error("_DisableDirectFlip 失败");
		}
	}

	// 在显示前设置窗口属性，其他程序应在缩放窗口显示后再检索窗口属性
	_SetWindowProps();

	// 缩放窗口可能有 WS_MAXIMIZE 样式，因此使用 SetWindowsPos 而不是 ShowWindow 
	// 以避免 OS 更改窗口尺寸和位置。
	SetWindowPos(
		_hWnd,
		NULL,
		_wndRect.left,
		_wndRect.top,
		_wndRect.right - _wndRect.left,
		_wndRect.bottom - _wndRect.top,
		SWP_SHOWWINDOW | SWP_NOCOPYBITS | SWP_NOREDRAW
	);

	// 为了方便调试，调试模式下使缩放窗口显示在源窗口下面
	if (_options.IsDebugMode()) {
		BringWindowToTop(_hwndSrc);
	}

	// 模拟独占全屏
	if (_options.IsSimulateExclusiveFullscreen()) {
		// 延迟 1s 以避免干扰游戏的初始化，见 #495
		([]()->winrt::fire_and_forget {
			ScalingWindow& that = ScalingWindow::Get();
			const HWND hwndScaling = that.Handle();
			winrt::DispatcherQueue dispatcher = that._dispatcher;

			co_await 1s;
			co_await dispatcher;

			if (that.Handle() != hwndScaling) {
				co_return;
			}

			if (!that._exclModeMutex) {
				that._exclModeMutex = ExclModeHelper::EnterExclMode();
			}
		})();
	};

	// 广播开始缩放
	PostMessage(HWND_BROADCAST, WM_MAGPIE_SCALINGCHANGED, 1, (LPARAM)_hWnd);

	return true;
}

void ScalingWindow::Render() noexcept {
	int srcState = _CheckSrcState();
	if (srcState != 0) {
		Logger::Get().Info("源窗口状态改变，退出全屏");
		// 切换前台窗口导致停止缩放时不应激活源窗口
		_renderer->SetOverlayVisibility(false, true);

		_isSrcRepositioning = srcState == 2;
		Destroy();
		return;
	}

	_cursorManager->Update();
	if (_renderer->Render()) {
		// 为了避免用户看到 DDF 窗口，在渲染第一帧后显示
		if (_hwndDDF && !_isDDFWindowShown) {
			ShowWindow(_hwndDDF, SW_NORMAL);
			SetWindowPos(_hwndDDF, Handle(), 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOREDRAW);
			_isDDFWindowShown = true;
		}
	}
}

void ScalingWindow::ToggleOverlay() noexcept {
	if (_renderer) {
		_renderer->SetOverlayVisibility(!_renderer->IsOverlayVisible());
	}
}

void ScalingWindow::RecreateAfterSrcRepositioned() noexcept {
	Create(_dispatcher, _hwndSrc, std::move(_options));
}

void ScalingWindow::CleanAfterSrcRepositioned() noexcept {
	_options = {};
	_hwndSrc = NULL;
	_dispatcher = nullptr;
	_isSrcRepositioning = false;
}

LRESULT ScalingWindow::_MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
	if (_renderer) {
		_renderer->MessageHandler(msg, wParam, lParam);
	}

	switch (msg) {
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	{
		if (_options.Is3DGameMode()) {
			break;
		}

		// 在以下情况下会收到光标消息:
		// 1、未捕获光标且缩放后的位置未被遮挡而缩放前的位置被遮挡
		// 2、光标位于叠加层上
		// 这时鼠标点击将激活源窗口
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
						INPUT inputs[]{
							INPUT{
								.type = INPUT_KEYBOARD,
								.ki = KEYBDINPUT{
									.wVk = VK_LWIN
								}
							},
							INPUT{
								.type = INPUT_KEYBOARD,
								.ki = KEYBDINPUT{
									.wVk = VK_LWIN,
									.dwFlags = KEYEVENTF_KEYUP
								}
							}
						};
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
		_exclModeMutex.reset();

		if (_hwndDDF) {
			DestroyWindow(_hwndDDF);
			_hwndDDF = NULL;
			_isDDFWindowShown = false;
		}

		_cursorManager.reset();
		_renderer.reset();
		_srcWndRect = {};

		// 如果正在源窗口正在调整，暂时不清理这些成员
		if (!_isSrcRepositioning) {
			_options = {};
			_hwndSrc = NULL;
			_dispatcher = nullptr;
		}

		// 还原时钟精度
		timeEndPeriod(1);

		// 广播停止缩放
		PostMessage(HWND_BROADCAST, WM_MAGPIE_SCALINGCHANGED, 0, 0);
		break;
	}
	default:
	{
		if (msg == WM_MAGPIE_TOUCHHELPER) {
			if (wParam == 1) {
				// 记录 TouchHelper 的结果
				if (lParam == 0) {
					Logger::Get().Info("触控输入变换设置成功");
				} else {
					Logger::Get().Error(fmt::format("触控输入变换设置失败\n\tLastErrorCode: {}", lParam));
				}
			}

			return 0;
		}
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

		// 3D 游戏模式下打开叠加层后如果源窗口意外回到前台应关闭叠加层
		if (_options.Is3DGameMode() && _renderer->IsOverlayVisible() && hwndForeground == _hwndSrc) {
			_renderer->SetOverlayVisibility(false, true);
		}

		// 在 3D 游戏模式下打开叠加层则全屏窗口可以接收焦点
		if (!_options.Is3DGameMode() || !_renderer->IsOverlayVisible() || hwndForeground != _hWnd) {
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
	// 检查所有者链是否存在 Magpie.ToolWindow 属性
	{
		HWND hWnd = hwndForeground;
		do {
			if (GetProp(hWnd, L"Magpie.ToolWindow")) {
				// 继续缩放
				return true;
			}

			hWnd = GetWindowOwner(hWnd);
		} while (hWnd);
	}

	if (WindowHelper::IsForbiddenSystemWindow(hwndForeground)) {
		return true;
	}

	RECT rectForground;
	if (!Win32Utils::GetWindowFrameRect(hwndForeground, rectForground)) {
		Logger::Get().Error("DwmGetWindowAttribute 失败");
		return false;
	}
	
	if (!IntersectRect(&rectForground, &rectForground, &_wndRect)) {
		// 没有重叠
		return true;
	}

	// 允许稍微重叠，减少意外停止缩放的机率
	SIZE rectSize = Win32Utils::GetSizeOfRect(rectForground);
	return rectSize.cx < 8 || rectSize.cy < 8;
}

bool ScalingWindow::_DisableDirectFlip(HINSTANCE hInstance) noexcept {
	// 没有显式关闭 DirectFlip 的方法
	// 将全屏窗口设为稍微透明，以灰色全屏窗口为背景

	static Utils::Ignore _ = [](HINSTANCE hInstance) {
		WNDCLASSEXW wcex{
			.cbSize = sizeof(wcex),
			.lpfnWndProc = DefWindowProc,
			.hInstance = hInstance,
			.hCursor = LoadCursor(nullptr, IDC_ARROW),
			.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH),
			.lpszClassName = CommonSharedConstants::DDF_WINDOW_CLASS_NAME
		};
		RegisterClassEx(&wcex);

		return Utils::Ignore();
	}(hInstance);

	_hwndDDF = CreateWindowEx(
		WS_EX_NOACTIVATE | WS_EX_LAYERED | WS_EX_TRANSPARENT,
		CommonSharedConstants::DDF_WINDOW_CLASS_NAME,
		NULL,
		WS_POPUP,
		_wndRect.left,
		_wndRect.top,
		_wndRect.right - _wndRect.left,
		_wndRect.bottom - _wndRect.top,
		NULL,
		NULL,
		hInstance,
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

	if (_renderer->FrameSource().IsScreenCapture()) {
		if (Win32Utils::GetOSVersion().Is20H1OrNewer()) {
			// 使 DDF 窗口无法被捕获到
			if (!SetWindowDisplayAffinity(_hwndDDF, WDA_EXCLUDEFROMCAPTURE)) {
				Logger::Get().Win32Error("SetWindowDisplayAffinity 失败");
			}
		}
	}

	return true;
}

// 用于和其他程序交互
void ScalingWindow::_SetWindowProps() const noexcept {
	SetProp(_hWnd, L"Magpie.SrcHWND", _hwndSrc);

	const RECT& srcRect = _renderer->SrcRect();
	SetProp(_hWnd, L"Magpie.SrcLeft", (HANDLE)(INT_PTR)srcRect.left);
	SetProp(_hWnd, L"Magpie.SrcTop", (HANDLE)(INT_PTR)srcRect.top);
	SetProp(_hWnd, L"Magpie.SrcRight", (HANDLE)(INT_PTR)srcRect.right);
	SetProp(_hWnd, L"Magpie.SrcBottom", (HANDLE)(INT_PTR)srcRect.bottom);

	const RECT& destRect = _renderer->DestRect();
	SetProp(_hWnd, L"Magpie.DestLeft", (HANDLE)(INT_PTR)destRect.left);
	SetProp(_hWnd, L"Magpie.DestTop", (HANDLE)(INT_PTR)destRect.top);
	SetProp(_hWnd, L"Magpie.DestRight", (HANDLE)(INT_PTR)destRect.right);
	SetProp(_hWnd, L"Magpie.DestBottom", (HANDLE)(INT_PTR)destRect.bottom);
}

}
