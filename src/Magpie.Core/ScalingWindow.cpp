#include "pch.h"
#include "ScalingWindow.h"
#include "CommonSharedConstants.h"
#include "Logger.h"
#include "Renderer.h"
#include "Win32Helper.h"
#include "WindowHelper.h"
#include "CursorManager.h"
#include <timeapi.h>
#include "FrameSourceBase.h"
#include "ExclModeHelper.h"
#include "StrHelper.h"
#include <dwmapi.h>
#include <ShellScalingApi.h>

namespace Magpie {

static UINT WM_MAGPIE_SCALINGCHANGED;
// 用于和 TouchHelper 交互
static UINT WM_MAGPIE_TOUCHHELPER;

static void InitMessage() noexcept {
	static Ignore _ = []() {
		WM_MAGPIE_SCALINGCHANGED =
			RegisterWindowMessage(CommonSharedConstants::WM_MAGPIE_SCALINGCHANGED);
		WM_MAGPIE_TOUCHHELPER =
			RegisterWindowMessage(CommonSharedConstants::WM_MAGPIE_TOUCHHELPER);

		return Ignore();
	}();
}


ScalingWindow::ScalingWindow() noexcept {}

ScalingWindow::~ScalingWindow() noexcept {}

// 获取窗口边框宽度
static uint32_t CalcWindowBorderThickness(HWND hWnd, const RECT& wndRect) noexcept {
	if (Win32Helper::GetWindowShowCmd(hWnd) != SW_SHOWNORMAL) {
		// 最大化的窗口不存在边框
		return 0;
	}

	// 检查该窗口是否禁用了非客户区域的绘制
	BOOL hasBorder = TRUE;
	HRESULT hr = DwmGetWindowAttribute(hWnd, DWMWA_NCRENDERING_ENABLED, &hasBorder, sizeof(hasBorder));
	if (FAILED(hr)) {
		Logger::Get().ComError("DwmGetWindowAttribute 失败", hr);
		return 0;
	}
	if (!hasBorder) {
		return 0;
	}

	RECT clientRect;
	if (!Win32Helper::GetClientScreenRect(hWnd, clientRect)) {
		Logger::Get().Win32Error("GetClientScreenRect 失败");
		return 0;
	}

	// 如果左右下三边均存在边框，那么应视为存在上边框:
	// * Win10 中窗口很可能绘制了假的上边框，这是很常见的创建无边框窗口的方法
	// * Win11 中 DWM 会将上边框绘制到客户区
	if (wndRect.top == clientRect.top && (wndRect.left == clientRect.left ||
		wndRect.right == clientRect.right || wndRect.bottom == clientRect.bottom)) {
		return 0;
	}

	if (Win32Helper::GetOSVersion().IsWin11()) {
		// Win11 的窗口边框宽度取决于 DPI
		uint32_t borderThickness = 0;
		hr = DwmGetWindowAttribute(hWnd, DWMWA_VISIBLE_FRAME_BORDER_THICKNESS, &borderThickness, sizeof(borderThickness));
		if (FAILED(hr)) {
			Logger::Get().ComError("DwmGetWindowAttribute 失败", hr);
			return 0;
		}

		return borderThickness;
	} else {
		// Win10 的窗口边框始终只有一个像素宽
		return 1;
	}
}

// 返回缩放窗口跨越的屏幕数量，失败返回 0
static uint32_t CalcFullscreenSwapChainRect(HWND hWnd, MultiMonitorUsage multiMonitorUsage, RECT& result) noexcept {
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

		if (Win32Helper::GetWindowShowCmd(hWnd) == SW_SHOWMAXIMIZED) {
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

			if (!Win32Helper::GetWindowFrameRect(hWnd, param.srcRect)) {
				Logger::Get().Error("GetWindowFrameRect 失败");
				return 0;
			}

			MONITORENUMPROC monitorEnumProc = [](HMONITOR, HDC, LPRECT monitorRect, LPARAM data) {
				MonitorEnumParam* param = (MonitorEnumParam*)data;

				if (Win32Helper::CheckOverlap(param->srcRect, *monitorRect)) {
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

ScalingError ScalingWindow::Create(
	const winrt::DispatcherQueue& dispatcher,
	HWND hwndSrc,
	ScalingOptions&& options
) noexcept {
	if (Handle()) {
		return ScalingError::ScalingFailedGeneral;
	}

	InitMessage();

#if _DEBUG
	OutputDebugString(fmt::format(L"可执行文件路径: {}\n窗口类: {}\n",
		Win32Helper::GetPathOfWnd(hwndSrc), Win32Helper::GetWndClassName(hwndSrc)).c_str());
#endif

	_hwndSrc = hwndSrc;
	// 缩放结束后才失效
	_options = std::move(options);
	_dispatcher = dispatcher;

	_isSrcFocused = false;
	_isSrcRepositioning = false;
	_runtimeError = ScalingError::NoError;

	if (FindWindow(CommonSharedConstants::SCALING_WINDOW_CLASS_NAME, nullptr)) {
		Logger::Get().Error("已存在缩放窗口");
		return ScalingError::ScalingFailedGeneral;
	}

	Logger::Get().Info(fmt::format("缩放开始\n\t程序版本: {}\n\t管理员: {}",
#ifdef MAGPIE_VERSION_TAG
		STRING(MAGPIE_VERSION_TAG),
#else
		"dev",
#endif
		Win32Helper::IsProcessElevated() ? "是" : "否"
	));

	// 记录缩放选项
	_options.Log();

	// 提高时钟精度，默认为 15.6ms
	timeBeginPeriod(1);

	if (_options.IsWindowedMode() || !_options.IsAllowScalingMaximized()) {
		if (Win32Helper::GetWindowShowCmd(_hwndSrc) == SW_SHOWMAXIMIZED) {
			Logger::Get().Info("源窗口已最大化");
			return ScalingError::Maximized;
		}
	}

	if (!GetWindowRect(hwndSrc, &_srcWndRect)) {
		Logger::Get().Win32Error("GetWindowRect 失败");
		return ScalingError::ScalingFailedGeneral;
	}

	_srcBorderThickness = CalcWindowBorderThickness(hwndSrc, _srcWndRect);

	static Ignore _ = []() {
		WNDCLASSEXW wcex{
			.cbSize = sizeof(wcex),
			.lpfnWndProc = _WndProc,
			.hInstance = wil::GetModuleInstanceHandle(),
			.hCursor = LoadCursor(nullptr, IDC_ARROW),
			.lpszClassName = CommonSharedConstants::SCALING_WINDOW_CLASS_NAME
		};
		RegisterClassEx(&wcex);

		// 不要直接使用 DefWindowProc，应确保窗口过程是 hInstance 模块里的函数
		wcex.lpfnWndProc = [](HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
			return DefWindowProc(hWnd, msg, wParam, lParam);
		};
		wcex.lpszClassName = CommonSharedConstants::SWAP_CHAIN_CHILD_WINDOW_CLASS_NAME;
		RegisterClassEx(&wcex);

		return Ignore();
	}();

	RECT windowRect{};
	HWND hwndSwapChain;
	if (_options.IsWindowedMode()) {
		_swapChainRect.top = _srcWndRect.top - 100;
		_swapChainRect.bottom = _srcWndRect.bottom + 100;
		SIZE srcWndSize = Win32Helper::GetSizeOfRect(_srcWndRect);
		long width = std::lroundf((_swapChainRect.bottom - _swapChainRect.top)
			* srcWndSize.cx / (float)srcWndSize.cy);
		_swapChainRect.left = _srcWndRect.left - (width - srcWndSize.cx) / 2;
		_swapChainRect.right = _swapChainRect.left + width;

		// 提前使用源窗口矩形创建缩放窗口以获取 DPI 和边框宽度
		CreateWindowEx(
			WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_NOREDIRECTIONBITMAP,
			CommonSharedConstants::SCALING_WINDOW_CLASS_NAME,
			L"Magpie",
			WS_OVERLAPPEDWINDOW,
			_srcWndRect.left,
			_srcWndRect.top,
			_srcWndRect.right - _srcWndRect.left,
			_srcWndRect.bottom - _srcWndRect.top,
			hwndSrc,
			NULL,
			wil::GetModuleInstanceHandle(),
			this
		);

		if (!Handle()) {
			Logger::Get().Error("创建缩放窗口失败");
			return ScalingError::ScalingFailedGeneral;
		}

		windowRect = _swapChainRect;
		AdjustWindowRectExForDpi(&windowRect, WS_OVERLAPPEDWINDOW, FALSE, 0, GetDpiForWindow(Handle()));

		// 为上边框预留空间
		uint32_t borderThickness = 0;
		if (_srcBorderThickness != 0) {
			if (Win32Helper::GetOSVersion().IsWin11()) {
				DwmGetWindowAttribute(Handle(), DWMWA_VISIBLE_FRAME_BORDER_THICKNESS, &borderThickness, sizeof(borderThickness));
			} else {
				borderThickness = 1;
			}
		}
		windowRect.top = _swapChainRect.top - borderThickness;

		Logger::Get().Info(fmt::format("缩放窗口矩形: {},{},{},{} ({}x{})",
			windowRect.left, windowRect.top, windowRect.right, windowRect.bottom,
			windowRect.right - windowRect.left, windowRect.bottom - windowRect.top));

		if (borderThickness == 0) {
			hwndSwapChain = Handle();
		} else {
			// 由于上边框的存在，交换链应使用子窗口。WS_EX_LAYERED | WS_EX_TRANSPARENT 使鼠标
			// 穿透子窗口，参见 https://learn.microsoft.com/en-us/windows/win32/winmsg/window-features#layered-windows
			hwndSwapChain = CreateWindowEx(
				WS_EX_NOREDIRECTIONBITMAP | WS_EX_LAYERED | WS_EX_TRANSPARENT,
				CommonSharedConstants::SWAP_CHAIN_CHILD_WINDOW_CLASS_NAME,
				L"",
				WS_CHILD | WS_VISIBLE,
				0,
				borderThickness,
				_swapChainRect.right - _swapChainRect.left,
				_swapChainRect.bottom - _swapChainRect.top,
				Handle(),
				NULL,
				wil::GetModuleInstanceHandle(),
				nullptr
			);
		}
	} else {
		uint32_t monitors = CalcFullscreenSwapChainRect(_hwndSrc, _options.multiMonitorUsage, _swapChainRect);
		if (monitors == 0) {
			Logger::Get().Error("CalcFullscreenSwapChainRect 失败");
			return ScalingError::ScalingFailedGeneral;
		}

		CreateWindowEx(
			WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_NOREDIRECTIONBITMAP,
			CommonSharedConstants::SCALING_WINDOW_CLASS_NAME,
			L"Magpie",
			WS_POPUP | (monitors == 1 ? WS_MAXIMIZE : 0),
			_swapChainRect.left,
			_swapChainRect.top,
			_swapChainRect.right - _swapChainRect.left,
			_swapChainRect.bottom - _swapChainRect.top,
			hwndSrc,
			NULL,
			wil::GetModuleInstanceHandle(),
			this
		);

		if (!Handle()) {
			Logger::Get().Error("创建缩放窗口失败");
			return ScalingError::ScalingFailedGeneral;
		}

		hwndSwapChain = Handle();
	}

	Logger::Get().Info(fmt::format("交换链矩形: {},{},{},{} ({}x{})",
		_swapChainRect.left, _swapChainRect.top, _swapChainRect.right, _swapChainRect.bottom,
		_swapChainRect.right - _swapChainRect.left, _swapChainRect.bottom - _swapChainRect.top));
	
	if (!_options.IsWindowedMode() && !_options.IsAllowScalingMaximized()) {
		// 源窗口和缩放窗口重合则不缩放，此时源窗口可能是无边框全屏窗口
		RECT srcRect;
		if (!Win32Helper::GetWindowFrameRect(_hwndSrc, srcRect)) {
			Logger::Get().Error("GetWindowFrameRect 失败");
			Destroy();
			return ScalingError::ScalingFailedGeneral;
		}

		if (srcRect == _swapChainRect) {
			Logger::Get().Info("源窗口已全屏");
			Destroy();
			return ScalingError::Maximized;
		}
	}

	if (_options.IsWindowedMode()) {
		BOOL value = TRUE;
		DwmSetWindowAttribute(Handle(), DWMWA_TRANSITIONS_FORCEDISABLED, &value, sizeof(value));

		// 如果源窗口不存在边框，缩放窗口也不应有边框。Win11 可以直接隐藏边框，Win10 则没有这么直接
		if (_srcBorderThickness == 0 && Win32Helper::GetOSVersion().IsWin11()) {
			COLORREF borderColor = DWMWA_COLOR_NONE;
			DwmSetWindowAttribute(Handle(), DWMWA_BORDER_COLOR, &borderColor, sizeof(borderColor));
		}
	}

	// 设置窗口不透明
	// 不完全透明时可关闭 DirectFlip
	if (!SetLayeredWindowAttributes(Handle(), 0, _options.IsDirectFlipDisabled() ? 254 : 255, LWA_ALPHA)) {
		Logger::Get().Win32Error("SetLayeredWindowAttributes 失败");
	}

	_renderer = std::make_unique<class Renderer>();
	ScalingError error = _renderer->Initialize(hwndSwapChain);
	if (error != ScalingError::NoError) {
		Logger::Get().Error("初始化 Renderer 失败");
		Destroy();
		return error;
	}

	_cursorManager = std::make_unique<class CursorManager>();
	_cursorManager->Initialize();

	if (_options.IsDirectFlipDisabled() && !_options.IsDebugMode()) {
		// 在此处创建的 DDF 窗口不会立刻显示
		if (!_DisableDirectFlip()) {
			Logger::Get().Error("_DisableDirectFlip 失败");
		}
	}

	if (_options.IsTouchSupportEnabled()) {
		_CreateTouchHoleWindows();
	}

	// 在显示前设置窗口属性，其他程序应在缩放窗口显示后再检索窗口属性
	_SetWindowProps();

	// 缩放窗口可能有 WS_MAXIMIZE 样式，因此使用 SetWindowsPos 而不是 ShowWindow 
	// 以避免 OS 更改窗口尺寸和位置。
	// 
	// SWP_NOACTIVATE 可以避免干扰 OS 内部的前台窗口历史，否则关闭开始菜单时不会自
	// 动激活源窗口。
	SetWindowPos(
		Handle(),
		NULL,
		windowRect.left,
		windowRect.top,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		SWP_SHOWWINDOW | SWP_NOACTIVATE | (_options.IsWindowedMode() ? 0 : SWP_NOMOVE | SWP_NOSIZE)
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
	PostMessage(HWND_BROADCAST, WM_MAGPIE_SCALINGCHANGED, 1, (LPARAM)Handle());

	for (const wil::unique_hwnd& hWnd : _hwndTouchHoles) {
		if (!hWnd) {
			continue;
		}

		SetWindowPos(hWnd.get(), Handle(), 0, 0, 0, 0,
			SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOREDRAW | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
	}

	return ScalingError::NoError;
}

void ScalingWindow::Render() noexcept {
	int srcState = _CheckSrcState();
	if (srcState > 1) {
		Logger::Get().Info("源窗口状态改变，退出全屏");
		// 切换前台窗口导致停止缩放时不应激活源窗口
		_renderer->SetOverlayVisibility(false, true);

		_isSrcRepositioning = srcState == 3;
		Destroy();
		return;
	}

	if (bool newIsSrcFocused = srcState == 0; newIsSrcFocused != _isSrcFocused) {
		_isSrcFocused = newIsSrcFocused;

		if (!ScalingWindow::Get().Options().IsWindowedMode()) {
			// 源窗口位于前台时将缩放窗口置顶，这使不支持 MPO 的显卡更容易激活 DirectFlip
			if (newIsSrcFocused) {
				SetWindowPos(Handle(), HWND_TOPMOST,
					0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
				// 再次调用 SetWindowPos 确保缩放窗口在所有置顶窗口之上
				SetWindowPos(Handle(), HWND_TOP,
					0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
			} else {
				SetWindowPos(Handle(), HWND_NOTOPMOST,
					0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
			}
		}
	}

	_cursorManager->Update();
	if (_renderer->Render()) {
		// 为了避免用户看到 DDF 窗口，在渲染第一帧后显示
		if (_hwndDDF && !_isDDFWindowShown) {
			SetWindowPos(_hwndDDF.get(), Handle(), 0, 0, 0, 0,
				SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
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
	case WM_CREATE:
	{
		// 源窗口的输入已被附加到了缩放窗口上，这是所有者窗口的默认行为，但我们不需要
		// 见 https://devblogs.microsoft.com/oldnewthing/20130412-00/?p=4683
		AttachThreadInput(
			GetCurrentThreadId(),
			GetWindowThreadProcessId(_hwndSrc, nullptr),
			FALSE
		);

		// 防止缩放 UWP 窗口时无法遮挡任务栏
		// https://github.com/dechamps/WindowInvestigator/issues/3
		SetProp(Handle(), L"TreatAsDesktopFullscreen", (HANDLE)TRUE);

		// TouchHelper 的权限可能比我们低
		if (!ChangeWindowMessageFilterEx(Handle(), WM_MAGPIE_TOUCHHELPER, MSGFLT_ADD, nullptr)) {
			Logger::Get().Win32Error("ChangeWindowMessageFilter 失败");
		}

		break;
	}
	case WM_NCCALCSIZE:
	{
		if (!_options.IsWindowedMode()) {
			break;
		}

		if (!wParam) {
			return 0;
		}

		NCCALCSIZE_PARAMS* params = (NCCALCSIZE_PARAMS*)lParam;
		RECT& clientRect = params->rgrc[0];

		// 保存原始上边框位置
		const LONG originalTop = clientRect.top;

		// 应用默认边框
		LRESULT ret = DefWindowProc(Handle(), WM_NCCALCSIZE, wParam, lParam);
		if (ret != 0) {
			return ret;
		}

		// 重新应用原始上边框，因此我们完全移除了默认边框中的上边框和标题栏，但保留了其他方向的边框
		clientRect.top = originalTop;
		return 0;
	}
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	{
		if (_options.Is3DGameMode()) {
			break;
		}

		// 在以下情况下会收到光标消息:
		// 1、未捕获光标且缩放后的位置未被遮挡而缩放前的位置被遮挡
		// 2、光标位于叠加层或黑边上
		// 这时鼠标点击将激活源窗口
		const HWND hwndForground = GetForegroundWindow();
		if (hwndForground != _hwndSrc) {
			if (!Win32Helper::SetForegroundWindow(_hwndSrc)) {
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
	case WM_MOUSEACTIVATE:
	{
		// 使得点击缩放窗口后关闭开始菜单能激活源窗口
		return MA_NOACTIVATE;
	}
	case WM_DESTROY:
	{
		Logger::Get().Info("缩放结束");

		if (_exclModeMutex) {
			_exclModeMutex.ReleaseMutex();
			_exclModeMutex.reset();
		}

		_hwndDDF.reset();
		_isDDFWindowShown = false;

		for (wil::unique_hwnd& hWnd : _hwndTouchHoles) {
			hWnd.reset();
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

		_RemoveWindowProps();

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

// 0 -> 前台窗口是源窗口且位置不变
// 1 -> 非 3D 游戏模式下前台窗口不是源窗口
// 2 -> 3D 游戏模式下前台窗口改变或源窗口最大化（如果不允许缩放最大化的窗口）/最小化
// 3 -> 源窗口大小或位置改变或最大化（如果允许缩放最大化的窗口）
int ScalingWindow::_CheckSrcState() const noexcept {
	if (!_options.IsDebugMode()) {
		const HWND hwndFore = GetForegroundWindow();

		if (_options.Is3DGameMode()) {
			if (_renderer->IsOverlayVisible()) {
				// 3D 游戏模式下打开叠加层后如果源窗口意外回到前台应关闭叠加层
				if (hwndFore == _hwndSrc) {
					_renderer->SetOverlayVisibility(false, true);
				}
			} else {
				// 在 3D 游戏模式下且没有打开叠加层时需检测前台窗口变化
				if (!_CheckForegroundFor3DGameMode(hwndFore)) {
					Logger::Get().Info("前台窗口已改变");
					return 2;
				}
			}
		} else {
			if (hwndFore != _hwndSrc) {
				return 1;
			}
		}
	}

	UINT showCmd = Win32Helper::GetWindowShowCmd(_hwndSrc);
	if (showCmd != SW_NORMAL && (showCmd != SW_SHOWMAXIMIZED || !_options.IsAllowScalingMaximized())) {
		Logger::Get().Info("源窗口显示状态改变");
		return 2;
	}

	RECT rect;
	if (!GetWindowRect(_hwndSrc, &rect)) {
		Logger::Get().Error("GetWindowRect 失败");
		return 2;
	}

	if (_srcWndRect != rect) {
		Logger::Get().Info("源窗口位置或大小改变");
		return 3;
	}

	return 0;
}

// 返回真表示应继续缩放
bool ScalingWindow::_CheckForegroundFor3DGameMode(HWND hwndFore) const noexcept {
	if (!hwndFore || hwndFore == _hwndSrc) {
		return true;
	}

	// 检查所有者链是否存在 Magpie.ToolWindow 属性
	{
		HWND hWnd = hwndFore;
		do {
			if (GetProp(hWnd, L"Magpie.ToolWindow")) {
				return true;
			}

			hWnd = GetWindowOwner(hWnd);
		} while (hWnd);
	}

	if (WindowHelper::IsForbiddenSystemWindow(hwndFore)) {
		return true;
	}

	RECT rectForground;
	if (!Win32Helper::GetWindowFrameRect(hwndFore, rectForground)) {
		Logger::Get().Error("DwmGetWindowAttribute 失败");
		return false;
	}
	
	if (!IntersectRect(&rectForground, &rectForground, &_swapChainRect)) {
		// 没有重叠
		return true;
	}

	// 允许稍微重叠，减少意外停止缩放的机率
	SIZE rectSize = Win32Helper::GetSizeOfRect(rectForground);
	return rectSize.cx < 8 || rectSize.cy < 8;
}

static LRESULT CALLBACK BkgWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_WINDOWPOSCHANGING) {
		// 确保始终在缩放窗口后
		((WINDOWPOS*)lParam)->hwndInsertAfter = ScalingWindow::Get().Handle();
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

bool ScalingWindow::_DisableDirectFlip() noexcept {
	// 没有显式关闭 DirectFlip 的方法
	// 将全屏窗口设为稍微透明，以灰色全屏窗口为背景

	static Ignore _ = []() {
		WNDCLASSEXW wcex{
			.cbSize = sizeof(wcex),
			.lpfnWndProc = BkgWndProc,
			.hInstance = wil::GetModuleInstanceHandle(),
			.hCursor = LoadCursor(nullptr, IDC_ARROW),
			.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH),
			.lpszClassName = CommonSharedConstants::DDF_WINDOW_CLASS_NAME
		};
		RegisterClassEx(&wcex);

		return Ignore();
	}();

	_hwndDDF.reset(CreateWindowEx(
		WS_EX_NOACTIVATE | WS_EX_LAYERED | WS_EX_TRANSPARENT,
		CommonSharedConstants::DDF_WINDOW_CLASS_NAME,
		NULL,
		WS_POPUP,
		_swapChainRect.left,
		_swapChainRect.top,
		_swapChainRect.right - _swapChainRect.left,
		_swapChainRect.bottom - _swapChainRect.top,
		NULL,
		NULL,
		wil::GetModuleInstanceHandle(),
		NULL
	));

	if (!_hwndDDF.get()) {
		Logger::Get().Win32Error("创建 DDF 窗口失败");
		return false;
	}

	// 设置窗口不透明
	if (!SetLayeredWindowAttributes(_hwndDDF.get(), 0, 255, LWA_ALPHA)) {
		Logger::Get().Win32Error("SetLayeredWindowAttributes 失败");
	}

	if (_renderer->FrameSource().IsScreenCapture()) {
		if (Win32Helper::GetOSVersion().Is20H1OrNewer()) {
			// 使 DDF 窗口无法被捕获到
			if (!SetWindowDisplayAffinity(_hwndDDF.get(), WDA_EXCLUDEFROMCAPTURE)) {
				Logger::Get().Win32Error("SetWindowDisplayAffinity 失败");
			}
		}
	}

	return true;
}

// 用于和其他程序交互
void ScalingWindow::_SetWindowProps() const noexcept {
	const HWND hWnd = Handle();
	SetProp(hWnd, L"Magpie.SrcHWND", _hwndSrc);

	const RECT& srcRect = _renderer->SrcRect();
	SetProp(hWnd, L"Magpie.SrcLeft", (HANDLE)(INT_PTR)srcRect.left);
	SetProp(hWnd, L"Magpie.SrcTop", (HANDLE)(INT_PTR)srcRect.top);
	SetProp(hWnd, L"Magpie.SrcRight", (HANDLE)(INT_PTR)srcRect.right);
	SetProp(hWnd, L"Magpie.SrcBottom", (HANDLE)(INT_PTR)srcRect.bottom);

	const RECT& destRect = _renderer->DestRect();
	SetProp(hWnd, L"Magpie.DestLeft", (HANDLE)(INT_PTR)destRect.left);
	SetProp(hWnd, L"Magpie.DestTop", (HANDLE)(INT_PTR)destRect.top);
	SetProp(hWnd, L"Magpie.DestRight", (HANDLE)(INT_PTR)destRect.right);
	SetProp(hWnd, L"Magpie.DestBottom", (HANDLE)(INT_PTR)destRect.bottom);
}

// 文档要求窗口被销毁前清理所有属性，但实际上这不是必须的，见
// https://devblogs.microsoft.com/oldnewthing/20231030-00/?p=108939
void ScalingWindow::_RemoveWindowProps() const noexcept {
	const HWND hWnd = Handle();
	RemoveProp(hWnd, L"Magpie.SrcHWND");
	RemoveProp(hWnd, L"Magpie.SrcLeft");
	RemoveProp(hWnd, L"Magpie.SrcTop");
	RemoveProp(hWnd, L"Magpie.SrcRight");
	RemoveProp(hWnd, L"Magpie.SrcBottom");
	RemoveProp(hWnd, L"Magpie.DestLeft");
	RemoveProp(hWnd, L"Magpie.DestTop");
	RemoveProp(hWnd, L"Magpie.DestRight");
	RemoveProp(hWnd, L"Magpie.DestBottom");

	if (_options.IsTouchSupportEnabled()) {
		RemoveProp(hWnd, L"Magpie.SrcTouchLeft");
		RemoveProp(hWnd, L"Magpie.SrcTouchTop");
		RemoveProp(hWnd, L"Magpie.SrcTouchRight");
		RemoveProp(hWnd, L"Magpie.SrcTouchBottom");
		RemoveProp(hWnd, L"Magpie.DestTouchLeft");
		RemoveProp(hWnd, L"Magpie.DestTouchTop");
		RemoveProp(hWnd, L"Magpie.DestTouchRight");
		RemoveProp(hWnd, L"Magpie.DestTouchBottom");
	}
}

// 在源窗口四周创建辅助窗口拦截黑边上的触控点击。
// 
// 直接将 srcRect 映射到 destRect 是天真的想法。似乎可以创建一个全屏的背景窗口来屏
// 蔽黑边，该方案的问题是无法解决源窗口和黑边的重叠部分。作为黑边，本应拦截用户点击，
// 但这也拦截了对源窗口的操作；若是不拦截会导致在黑边上可以操作源窗口。
// 
// 我们的方案是：将源窗口和其周围映射到整个缩放窗口，并在源窗口四周创建背景窗口拦截
// 对黑边的点击。注意这些背景窗口不能由 TouchHelper.exe 创建，因为它有 UIAccess
// 权限，创建的窗口会遮盖缩放窗口。
void ScalingWindow::_CreateTouchHoleWindows() noexcept {
	// 将黑边映射到源窗口
	const RECT& srcRect = _renderer->SrcRect();
	const RECT& destRect = _renderer->DestRect();

	const double scaleX = double(destRect.right - destRect.left) / (srcRect.right - srcRect.left);
	const double scaleY = double(destRect.bottom - destRect.top) / (srcRect.bottom - srcRect.top);

	RECT srcTouchRect = srcRect;

	if (destRect.left > _swapChainRect.left) {
		srcTouchRect.left -= lround((destRect.left - _swapChainRect.left) / scaleX);
	}
	if (destRect.top > _swapChainRect.top) {
		srcTouchRect.top -= lround((destRect.top - _swapChainRect.top) / scaleX);
	}
	if (destRect.right < _swapChainRect.right) {
		srcTouchRect.right += lround((_swapChainRect.right - destRect.right) / scaleY);
	}
	if (destRect.bottom < _swapChainRect.bottom) {
		srcTouchRect.bottom += lround((_swapChainRect.bottom - destRect.bottom) / scaleY);
	}

	static Ignore _ = []() {
		WNDCLASSEXW wcex{
			.cbSize = sizeof(wcex),
			.lpfnWndProc = BkgWndProc,
			.hInstance = wil::GetModuleInstanceHandle(),
			.lpszClassName = CommonSharedConstants::TOUCH_HELPER_HOLE_WINDOW_CLASS_NAME
		};
		RegisterClassEx(&wcex);

		return Ignore();
	}();

	const auto createHoleWindow = [&](uint32_t idx, LONG left, LONG top, LONG right, LONG bottom) noexcept {
		_hwndTouchHoles[idx].reset(CreateWindowEx(
			WS_EX_NOREDIRECTIONBITMAP | WS_EX_NOACTIVATE,
			CommonSharedConstants::TOUCH_HELPER_HOLE_WINDOW_CLASS_NAME,
			nullptr,
			WS_POPUP,
			left,
			top,
			right - left,
			bottom - top,
			NULL,
			NULL,
			wil::GetModuleInstanceHandle(),
			0
		));
	};

	//      srcTouchRect
	// ┌───┬───────────┬───┐
	// │   │     1     │   │
	// │   ├───────────┤   │
	// │   │           │   │
	// │ 0 │  srcRect  │ 2 │
	// │   │           │   │
	// │   ├───────────┤   │
	// │   │     3     │   │
	// └───┴───────────┴───┘

	if (srcRect.left > srcTouchRect.left) {
		createHoleWindow(0, srcTouchRect.left, srcTouchRect.top, srcRect.left, srcTouchRect.bottom);
	}
	if (srcRect.top > srcTouchRect.top) {
		createHoleWindow(1, srcRect.left, srcTouchRect.top, srcRect.right, srcRect.top);
	}
	if (srcRect.right < srcTouchRect.right) {
		createHoleWindow(2, srcRect.right, srcTouchRect.top, srcTouchRect.right, srcTouchRect.bottom);
	}
	if (srcRect.bottom < srcTouchRect.bottom) {
		createHoleWindow(3, srcRect.left, srcRect.bottom, srcRect.right, srcTouchRect.bottom);
	}

	// 供 TouchHelper.exe 使用
	const HWND hWnd = Handle();
	SetProp(hWnd, L"Magpie.SrcTouchLeft", (HANDLE)(INT_PTR)srcTouchRect.left);
	SetProp(hWnd, L"Magpie.SrcTouchTop", (HANDLE)(INT_PTR)srcTouchRect.top);
	SetProp(hWnd, L"Magpie.SrcTouchRight", (HANDLE)(INT_PTR)srcTouchRect.right);
	SetProp(hWnd, L"Magpie.SrcTouchBottom", (HANDLE)(INT_PTR)srcTouchRect.bottom);

	SetProp(hWnd, L"Magpie.DestTouchLeft", (HANDLE)(INT_PTR)_swapChainRect.left);
	SetProp(hWnd, L"Magpie.DestTouchTop", (HANDLE)(INT_PTR)_swapChainRect.top);
	SetProp(hWnd, L"Magpie.DestTouchRight", (HANDLE)(INT_PTR)_swapChainRect.right);
	SetProp(hWnd, L"Magpie.DestTouchBottom", (HANDLE)(INT_PTR)_swapChainRect.bottom);
}

}
