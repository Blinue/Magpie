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

// 窗口模式缩放时缩放窗口应遮挡源窗口和它的阴影，在四周留出 50 x DPI缩放 的空间
static constexpr int WINDOWED_MODE_MIN_SPACE_AROUND = 2 * 50;

static void InitMessage() noexcept {
	static Ignore _ = []() {
		WM_MAGPIE_SCALINGCHANGED =
			RegisterWindowMessage(CommonSharedConstants::WM_MAGPIE_SCALINGCHANGED);
		WM_MAGPIE_TOUCHHELPER =
			RegisterWindowMessage(CommonSharedConstants::WM_MAGPIE_TOUCHHELPER);

		return Ignore();
	}();
}

ScalingWindow::ScalingWindow() noexcept :
	_resourceLoader(winrt::ResourceLoader::GetForViewIndependentUse(CommonSharedConstants::APP_RESOURCE_MAP_ID)) {}

ScalingWindow::~ScalingWindow() noexcept {}

ScalingError ScalingWindow::Create(
	HWND hwndSrc,
	winrt::DispatcherQueue dispatcher,
	ScalingOptions options
) noexcept {
	if (Handle()) {
		return ScalingError::ScalingFailedGeneral;
	}

	InitMessage();

#if _DEBUG
	OutputDebugString(fmt::format(L"可执行文件路径: {}\n窗口类: {}\n",
		Win32Helper::GetPathOfWnd(hwndSrc), Win32Helper::GetWndClassName(hwndSrc)).c_str());
#endif

	// 缩放结束后失效
	_dispatcher = std::move(dispatcher);
	_options = std::move(options);
	_runtimeError = ScalingError::NoError;
	_isResizingOrMoving = false;

	if (FindWindow(CommonSharedConstants::SCALING_WINDOW_CLASS_NAME, nullptr)) {
		Logger::Get().Error("已存在缩放窗口");
		return ScalingError::ScalingFailedGeneral;
	}

	Logger::Get().Info(fmt::format("缩放开始\n\t程序版本: {}\n\tOS 版本: {}\n\t管理员: {}",
#ifdef MAGPIE_VERSION_TAG
		STRING(MAGPIE_VERSION_TAG),
#else
		"dev",
#endif
		Win32Helper::GetOSVersion().ToString<char>(),
		Win32Helper::IsProcessElevated() ? "是" : "否"
	));

	if (ScalingError error = _srcInfo.Set(hwndSrc, _options); error != ScalingError::NoError) {
		Logger::Get().Error("初始化 SrcInfo 失败");
		return error;
	}

	if (_srcInfo.IsZoomed()) {
		if (!_options.IsAllowScalingMaximized()) {
			Logger::Get().Info("源窗口已最大化");
			return ScalingError::Maximized;
		} else if (_options.IsWindowedMode()) {
			Logger::Get().Info("已最大化的窗口不支持窗口模式缩放");
			return ScalingError::BannedInWindowedMode;
		}
	}

	static Ignore _ = []() {
		WNDCLASSEXW wcex{
			.cbSize = sizeof(wcex),
			.lpfnWndProc = _WndProc,
			.hInstance = wil::GetModuleInstanceHandle(),
			.hCursor = LoadCursor(nullptr, IDC_ARROW),
			.lpszClassName = CommonSharedConstants::SCALING_WINDOW_CLASS_NAME
		};
		RegisterClassEx(&wcex);

		wcex.lpfnWndProc = _RendererWndProc;
		wcex.lpszClassName = CommonSharedConstants::RENDERER_CHILD_WINDOW_CLASS_NAME;
		RegisterClassEx(&wcex);

		return Ignore();
	}();

	const SrcWindowKind srcWindowKind = _srcInfo.WindowKind();
	const bool isWin11 = Win32Helper::GetOSVersion().IsWin11();
	// 不存在非客户区，渲染无需创建在子窗口里
	const bool isAllClient = !isWin11 &&
		(srcWindowKind == SrcWindowKind::NoBorder || srcWindowKind == SrcWindowKind::NoDecoration);
	if (_options.IsWindowedMode()) {
		const RECT& srcFrameRect = _srcInfo.WindowFrameRect();
		const RECT& srcRect = _srcInfo.SrcRect();

		LONG rendererHeight = srcFrameRect.bottom - srcFrameRect.top + 200;
		LONG rendererWidth = (LONG)std::lroundf(rendererHeight * (srcRect.right - srcRect.left)
			/ float(srcRect.bottom - srcRect.top));

		SIZE windowSize;
		LONG leftPadding = 0;
		if (isAllClient) {
			_topBorderThicknessInClient = 0;
			_nonTopBorderThicknessInClient = 0;
			windowSize = { rendererWidth, rendererHeight };
		} else {
			const POINT windwoCenter{
				(srcFrameRect.left + srcFrameRect.right) / 2,
				(srcFrameRect.top + srcFrameRect.bottom) / 2
			};
			HMONITOR hMon = MonitorFromPoint(windwoCenter, MONITOR_DEFAULTTONEAREST);
			GetDpiForMonitor(hMon, MDT_EFFECTIVE_DPI, &_currentDpi, &_currentDpi);

			if (isWin11 && srcWindowKind == SrcWindowKind::NoDecoration) {
				// NoDecoration 在 Win11 中和 NoTitleBar 一样处理并禁用边框，优点是只需一个辅助窗口
				_topBorderThicknessInClient = 0;
			} else {
				_topBorderThicknessInClient = Win32Helper::GetNativeWindowBorderThickness(_currentDpi);
			}

			if (_srcInfo.WindowKind() == SrcWindowKind::NoBorder) {
				// Win11 中为客户区内的边框预留空间
				assert(isWin11);
				_nonTopBorderThicknessInClient = _topBorderThicknessInClient;

				windowSize = {
					rendererWidth + 2 * (LONG)_topBorderThicknessInClient,
					rendererHeight + 2 * (LONG)_topBorderThicknessInClient
				};
				leftPadding = _topBorderThicknessInClient;
			} else {
				_nonTopBorderThicknessInClient = 0;

				RECT rect = { 0,0,rendererWidth,rendererHeight };
				AdjustWindowRectExForDpi(&rect, WS_OVERLAPPEDWINDOW, FALSE, 0, _currentDpi);
				// 不计标题栏
				windowSize = { rect.right - rect.left,rect.bottom };
				leftPadding = -rect.left;

				// 为上边框预留空间
				windowSize.cy += _topBorderThicknessInClient;
			}
		}

		_windowRect.left = srcFrameRect.left - (windowSize.cx - (srcFrameRect.right - srcFrameRect.left)) / 2;
		_windowRect.top = srcFrameRect.top - (windowSize.cy - (srcFrameRect.bottom - srcFrameRect.top)) / 2;
		_windowRect.right = _windowRect.left + windowSize.cx;
		_windowRect.bottom = _windowRect.top + windowSize.cy;

		Logger::Get().Info(fmt::format("缩放窗口矩形: {},{},{},{} ({}x{})",
			_windowRect.left, _windowRect.top, _windowRect.right, _windowRect.bottom,
			_windowRect.right - _windowRect.left, _windowRect.bottom - _windowRect.top));

		CreateWindowEx(
			WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_NOREDIRECTIONBITMAP,
			CommonSharedConstants::SCALING_WINDOW_CLASS_NAME,
			nullptr,
			WS_OVERLAPPED | WS_CAPTION | WS_THICKFRAME,
			_windowRect.left,
			_windowRect.top,
			_windowRect.right - _windowRect.left,
			_windowRect.bottom - _windowRect.top,
			hwndSrc,
			NULL,
			wil::GetModuleInstanceHandle(),
			this
		);

		if (!Handle()) {
			Logger::Get().Error("创建缩放窗口失败");
			return ScalingError::ScalingFailedGeneral;
		}

		if (isAllClient) {
			_rendererRect = _windowRect;
			_hwndRenderer = Handle();
		} else {
			_rendererRect.left = _windowRect.left + leftPadding;
			_rendererRect.top = _windowRect.top + _topBorderThicknessInClient;
			_rendererRect.right = _rendererRect.left + rendererWidth;
			_rendererRect.bottom = _rendererRect.top + rendererHeight;

			// 由于边框的存在，渲染应使用子窗口。WS_EX_LAYERED | WS_EX_TRANSPARENT 使鼠标
			// 穿透子窗口，参见 https://learn.microsoft.com/en-us/windows/win32/winmsg/window-features#layered-windows
			_hwndRenderer = CreateWindowEx(
				WS_EX_NOREDIRECTIONBITMAP | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOPARENTNOTIFY,
				CommonSharedConstants::RENDERER_CHILD_WINDOW_CLASS_NAME,
				nullptr,
				WS_CHILD | WS_VISIBLE,
				_nonTopBorderThicknessInClient,
				_topBorderThicknessInClient,
				rendererWidth,
				rendererHeight,
				Handle(),
				NULL,
				wil::GetModuleInstanceHandle(),
				nullptr
			);
		}
	} else {
		uint32_t monitorCount;
		ScalingError error = _CalcFullscreenRendererRect(monitorCount);
		if (error != ScalingError::NoError) {
			Logger::Get().Error("_CalcFullscreenRendererRect 失败");
			return error;
		}

		_topBorderThicknessInClient = 0;
		_nonTopBorderThicknessInClient = 0;
		_windowRect = _rendererRect;

		CreateWindowEx(
			WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_NOREDIRECTIONBITMAP,
			CommonSharedConstants::SCALING_WINDOW_CLASS_NAME,
			nullptr,
			WS_POPUP | (monitorCount == 1 ? WS_MAXIMIZE : 0),
			_windowRect.left,
			_windowRect.top,
			_windowRect.right - _windowRect.left,
			_windowRect.bottom - _windowRect.top,
			hwndSrc,
			NULL,
			wil::GetModuleInstanceHandle(),
			this
		);

		if (!Handle()) {
			Logger::Get().Error("创建缩放窗口失败");
			return ScalingError::ScalingFailedGeneral;
		}

		_hwndRenderer = Handle();
	}

	Logger::Get().Info(fmt::format("渲染矩形: {},{},{},{} ({}x{})",
		_rendererRect.left, _rendererRect.top, _rendererRect.right, _rendererRect.bottom,
		_rendererRect.right - _rendererRect.left, _rendererRect.bottom - _rendererRect.top));
	
	if (!_options.IsWindowedMode() && !_options.IsAllowScalingMaximized()) {
		// 检查源窗口是否是无边框全屏窗口
		if (srcWindowKind == SrcWindowKind::NoDecoration && _srcInfo.WindowRect() == _rendererRect) {
			Logger::Get().Info("源窗口已全屏");
			Destroy();
			return ScalingError::Maximized;
		}
	}

	_renderer = std::make_unique<class Renderer>();
	ScalingError error = _renderer->Initialize(_hwndRenderer, _options.overlayOptions);
	if (error != ScalingError::NoError) {
		Logger::Get().Error("初始化 Renderer 失败");
		Destroy();
		return error;
	}

	_cursorManager = std::make_unique<class CursorManager>();
	_cursorManager->Initialize();

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
		0, 0, 0, 0,
		SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE
	);

	// 如果源窗口位于前台则将缩放窗口置顶
	if (_srcInfo.IsFocused()) {
		_UpdateFocusState();
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
	const bool originIsSrcFocused = _srcInfo.IsFocused();

	if (!_CheckSrcState()) {
		Logger::Get().Info("源窗口状态改变，停止缩放");
		Destroy();
		return;
	}

	if (_srcInfo.IsFocused() != originIsSrcFocused) {
		_UpdateFocusState();
	}

	_cursorManager->Update();
	_renderer->Render();
}

void ScalingWindow::ToggleToolbarState() noexcept {
	if (_renderer) {
		_renderer->ToggleToolbarState();
	}
}

void ScalingWindow::RecreateAfterSrcRepositioned() noexcept {
	_isSrcRepositioning = false;
	Create(_srcInfo.Handle(), _dispatcher, std::move(_options));
}

void ScalingWindow::CleanAfterSrcRepositioned() noexcept {
	_dispatcher = nullptr;
	_options = {};
	_isSrcRepositioning = false;
}

winrt::hstring ScalingWindow::GetLocalizedString(std::wstring_view resName) const {
	return _resourceLoader.GetString(resName);
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
			GetWindowThreadProcessId(_srcInfo.Handle(), nullptr),
			FALSE
		);

		// 防止缩放 UWP 窗口时无法遮挡任务栏
		// https://github.com/dechamps/WindowInvestigator/issues/3
		SetProp(Handle(), L"TreatAsDesktopFullscreen", (HANDLE)TRUE);

		// TouchHelper 的权限可能比我们低
		if (!ChangeWindowMessageFilterEx(Handle(), WM_MAGPIE_TOUCHHELPER, MSGFLT_ADD, nullptr)) {
			Logger::Get().Win32Error("ChangeWindowMessageFilter 失败");
		}

		_currentDpi = GetDpiForWindow(Handle());

		// 设置窗口不透明。不完全透明时可关闭 DirectFlip
		if (!SetLayeredWindowAttributes(Handle(), 0, 255, LWA_ALPHA)) {
			Logger::Get().Win32Error("SetLayeredWindowAttributes 失败");
		}

		if (_options.IsWindowedMode()) {
			BOOL value = TRUE;
			DwmSetWindowAttribute(Handle(), DWMWA_TRANSITIONS_FORCEDISABLED, &value, sizeof(value));

			if (_IsBorderless()) {
				// 保留窗口阴影
				MARGINS margins{ 1,1,1,1 };
				DwmExtendFrameIntoClientArea(Handle(), &margins);
			}

			if (_srcInfo.WindowKind() == SrcWindowKind::NoDecoration && Win32Helper::GetOSVersion().IsWin11()) {
				// Win11 中禁用边框和圆角以模仿 NoDecoration 的样式
				COLORREF color = DWMWA_COLOR_NONE;
				DwmSetWindowAttribute(Handle(), DWMWA_BORDER_COLOR, &color, sizeof(color));

				DWM_WINDOW_CORNER_PREFERENCE corner = DWMWCP_DONOTROUND;
				DwmSetWindowAttribute(Handle(), DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof(corner));
			}

			_UpdateFrameMargins();

			// 创建用于在窗口外调整尺寸的辅助窗口
			_CreateBorderHelperWindows();
			_RepostionBorderHelperWindows();
		}

		// 提高时钟精度，默认为 15.6ms。缩放窗口销毁时
		timeBeginPeriod(1);
		break;
	}
	case CommonSharedConstants::WM_FRONTEND_RENDER:
	{
		// 调整窗口大小时会进入 OS 的内部循环，我们的消息循环没有机会调用 Render。幸运的是
		// 内部循环会正常分发消息，因此有必要在窗口过程中执行渲染以避免调整大小时渲染暂停。
		Render();
		return 0;
	}
	case WM_ENTERSIZEMOVE:
	{
		_isResizingOrMoving = true;
		return 0;
	}
	case WM_EXITSIZEMOVE:
	{
		_isResizingOrMoving = false;
		_renderer->EndResize();
		return 0;
	}
	case WM_DPICHANGED:
	{
		_currentDpi = HIWORD(wParam);
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

		_UpdateFrameMargins();

		if (!_IsBorderless()) {
			// 应用默认边框
			LRESULT ret = DefWindowProc(Handle(), WM_NCCALCSIZE, wParam, lParam);
			if (ret != 0) {
				return ret;
			}

			// 重新应用原始上边框，因此我们完全移除了默认边框中的上边框和标题栏，但保留了其他方向的边框
			RECT& clientRect = ((NCCALCSIZE_PARAMS*)lParam)->rgrc[0];
			clientRect.top = _windowRect.top;
		}
		
		return 0;
	}
	case WM_NCHITTEST:
	{
		if (!_options.IsWindowedMode()) {
			break;
		}

		if (_cursorManager->IsCursorOnSrcTitleBar() || _renderer->IsCursorOnOverlayCaptionArea()) {
			// 鼠标在源窗口的标题栏上或叠加层工具栏上时可以拖动缩放窗口
			return HTCAPTION;
		}

		break;
	}
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	{
		// 在以下情况下会收到光标消息:
		// 1、未捕获光标且缩放后的位置未被遮挡而缩放前的位置被遮挡
		// 2、光标位于叠加层或黑边上
		// 这时鼠标点击将激活源窗口
		const HWND hwndForground = GetForegroundWindow();
		if (hwndForground != _srcInfo.Handle()) {
			if (!Win32Helper::SetForegroundWindow(_srcInfo.Handle())) {
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

					SetForegroundWindow(_srcInfo.Handle());
				}
			}
		}
		break;
	}
	case WM_NCACTIVATE:
	{
		wParam = _srcInfo.IsFocused();
		break;
	}
	case WM_MOUSEACTIVATE:
	{
		// 使得点击缩放窗口后关闭开始菜单能激活源窗口
		return MA_NOACTIVATE;
	}
	case WM_SIZING:
	{
		if (!_options.IsWindowedMode()) {
			break;
		}

		// 确保调整尺寸时渲染窗口长宽比不变，且需要限制最小和最大尺寸

		RECT& windowRect = *(RECT*)lParam;

		const RECT& srcRect = _srcInfo.SrcRect();
		const float srcAspectRatio = float(srcRect.bottom - srcRect.top) / (srcRect.right - srcRect.left);
		
		// 计算最小尺寸时使用源窗口包含窗口框架的矩形而不是被缩放区域
		const RECT& srcFrameRect = _srcInfo.WindowFrameRect();
		const int spaceAround = (int)lroundf(WINDOWED_MODE_MIN_SPACE_AROUND *
			_currentDpi / float(USER_DEFAULT_SCREEN_DPI));
		const int minRendererWidth = srcFrameRect.right - srcFrameRect.left + spaceAround;
		const int minRendererHeight = srcFrameRect.bottom - srcFrameRect.top + spaceAround;

		int xExtraSpace = 0;
		int yExtraSpace = 0;
		if (_IsBorderless()) {
			xExtraSpace = 2 * _nonTopBorderThicknessInClient;
			yExtraSpace = _topBorderThicknessInClient + _nonTopBorderThicknessInClient;
		} else {
			RECT rect{};
			AdjustWindowRectExForDpi(&rect, WS_OVERLAPPEDWINDOW, FALSE, 0, _currentDpi);
			xExtraSpace = rect.right - rect.left;
			yExtraSpace = _topBorderThicknessInClient + rect.bottom;
		}

		int rendererWidth;
		int rendererHeight;
		if (wParam == WMSZ_TOP || wParam == WMSZ_BOTTOM) {
			// 上下两边上调整尺寸时宽度随高度变化
			rendererHeight = (windowRect.bottom - windowRect.top) - yExtraSpace;
			rendererWidth = (int)std::lroundf(rendererHeight / srcAspectRatio);
		} else {
			// 其他边上调整尺寸时使用高度随宽度变化
			rendererWidth = (windowRect.right - windowRect.left) - xExtraSpace;
			rendererHeight = (int)std::lroundf(rendererWidth * srcAspectRatio);
		}

		// 确保渲染窗口比源窗口稍大
		if (rendererWidth > rendererHeight) {
			if (rendererHeight < minRendererHeight) {
				rendererHeight = minRendererHeight;
				rendererWidth = (int)std::lroundf(rendererHeight / srcAspectRatio);
			}
		} else {
			if (rendererWidth < minRendererWidth) {
				rendererWidth = minRendererWidth;
				rendererHeight = (int)std::lroundf(rendererWidth * srcAspectRatio);
			}
		}

		int windowWidth = rendererWidth + xExtraSpace;
		int windowHeight = rendererHeight + yExtraSpace;

		// 确保缩放窗口尺寸不超过系统限制
		const int maxWidth = GetSystemMetricsForDpi(SM_CXMAXTRACK, _currentDpi);
		const int maxHeight = GetSystemMetricsForDpi(SM_CYMAXTRACK, _currentDpi);
		if (windowWidth > maxWidth || windowHeight > maxHeight) {
			// 尝试最大宽度，失败则使用最大高度
			int testHeight = (int)std::lroundf((maxWidth - xExtraSpace) * srcAspectRatio) + yExtraSpace;
			if (testHeight < maxHeight) {
				windowWidth = maxWidth;
				windowHeight = testHeight;
			} else {
				windowHeight = maxHeight;
				windowWidth = (int)std::lroundf((maxHeight - yExtraSpace) / srcAspectRatio) + xExtraSpace;
			}
		}

		if (wParam == WMSZ_LEFT || wParam == WMSZ_TOPLEFT || wParam == WMSZ_BOTTOMLEFT) {
			windowRect.left = windowRect.right - windowWidth;
		} else {
			windowRect.right = windowRect.left + windowWidth;
		}

		if (wParam == WMSZ_TOP || wParam == WMSZ_TOPLEFT || wParam == WMSZ_TOPRIGHT) {
			windowRect.top = windowRect.bottom - windowHeight;
		} else {
			windowRect.bottom = windowRect.top + windowHeight;
		}

		return TRUE;
	}
	case WM_WINDOWPOSCHANGING:
	{
		if (_options.IsWindowedMode()) {
			// 缩放窗口位置或尺寸有变化则调整源窗口位置，使源窗口始终被缩放窗口遮盖
			WINDOWPOS& windowPos = *(WINDOWPOS*)lParam;
			if ((windowPos.flags & (SWP_NOSIZE | SWP_NOMOVE)) != (SWP_NOSIZE | SWP_NOMOVE)) {
				_windowRect.left = windowPos.x;
				_windowRect.top = windowPos.y;
				_windowRect.right = windowPos.x + windowPos.cx;
				_windowRect.bottom = windowPos.y + windowPos.cy;

				const SIZE oldRendererSize = Win32Helper::GetSizeOfRect(_rendererRect);

				// 计算渲染矩形
				if (_IsBorderless()) {
					_rendererRect.left = windowPos.x + _nonTopBorderThicknessInClient;
					_rendererRect.top = windowPos.y + _topBorderThicknessInClient;
					_rendererRect.right = windowPos.x + windowPos.cx - _nonTopBorderThicknessInClient;
					_rendererRect.bottom = windowPos.y + windowPos.cy - _nonTopBorderThicknessInClient;
				} else {
					RECT frameRect{};
					AdjustWindowRectExForDpi(&frameRect, WS_OVERLAPPEDWINDOW, FALSE, 0, _currentDpi);

					_rendererRect.left = windowPos.x - frameRect.left;
					_rendererRect.top = windowPos.y + _topBorderThicknessInClient;
					_rendererRect.right = windowPos.x + windowPos.cx - frameRect.right;
					_rendererRect.bottom = windowPos.y + windowPos.cy - frameRect.bottom;
				}

				const bool resized = Win32Helper::GetSizeOfRect(_rendererRect) != oldRendererSize;

				if (_hwndRenderer == Handle()) {
					if (resized) {
						// 为了平滑调整窗口尺寸，渲染所在窗口需要在 WM_WINDOWPOSCHANGING 中
						// 更新渲染尺寸。
						_ResizeRenderer();
					} else {
						_MoveRenderer();
					}
				} else {
					// 渲染口过程将在 WM_WINDOWPOSCHANGING 中更新渲染尺寸
					SetWindowPos(
						_hwndRenderer,
						NULL,
						_nonTopBorderThicknessInClient,
						_topBorderThicknessInClient,
						_rendererRect.right - _rendererRect.left,
						_rendererRect.bottom - _rendererRect.top,
						SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOZORDER | (resized ? 0 : SWP_NOSIZE)
					);
				}

				// 将源窗口移到渲染窗口中心
				const SIZE rendererSize = Win32Helper::GetSizeOfRect(_rendererRect);
				const RECT& srcFrameRect = _srcInfo.WindowFrameRect();
				const SIZE srcFreamRect = Win32Helper::GetSizeOfRect(srcFrameRect);

				int offsetX = _rendererRect.left + (rendererSize.cx - srcFreamRect.cx) / 2 - srcFrameRect.left;
				int offsetY = _rendererRect.top + (rendererSize.cy - srcFreamRect.cy) / 2 - srcFrameRect.top;
				_MoveSrcWindow(offsetX, offsetY);

				_RepostionBorderHelperWindows();
			}
		}

		return 0;
	}
	case WM_WINDOWPOSCHANGED:
	{
		if (_options.IsWindowedMode()) {
			// WS_EX_NOACTIVATE 和处理 WM_MOUSEACTIVATE 仍然无法完全阻止缩放窗口接收
			// 焦点。进行下面的操作：调整缩放窗口尺寸，打开开始菜单然后关闭，缩放窗口便
			// 得到焦点了。这应该是 OS 的 bug，下面的代码用于规避它。
			if (!(((WINDOWPOS*)lParam)->flags & SWP_NOACTIVATE)) {
				Win32Helper::SetForegroundWindow(_srcInfo.Handle());
			}
		}

		// 不调用 DefWindowProc，因此不会收到 WM_SIZE 和 WM_MOVE
		return 0;
	}
	case WM_SYSCOMMAND:
	{
		if ((wParam & 0xFFF0) == SC_SIZE) {
			Win32Helper::SetForegroundWindow(_srcInfo.Handle());
		}
		break;
	}
	case WM_DESTROY:
	{
		Logger::Get().Info("缩放结束");

		if (_exclModeMutex) {
			_exclModeMutex.ReleaseMutex();
			_exclModeMutex.reset();
		}

		for (wil::unique_hwnd& hWnd : _hwndTouchHoles) {
			hWnd.reset();
		}

		_cursorManager.reset();
		_renderer.reset();

		// 如果正在源窗口正在调整，暂时不清理这些成员
		if (!_isSrcRepositioning) {
			_dispatcher = nullptr;
			// 缩放结束时保存配置
			_options.save(_options, NULL);
			_options = {};
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

LRESULT ScalingWindow::_RendererWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_WINDOWPOSCHANGING) {
		WINDOWPOS& windowPos = *(WINDOWPOS*)lParam;
		if (!(windowPos.flags & SWP_NOSIZE)) {
			// 为了平滑调整窗口尺寸，渲染所在窗口需要在 WM_WINDOWPOSCHANGING 中
			// 更新渲染尺寸。
			Get()._ResizeRenderer();
		} else if (!(windowPos.flags & SWP_NOMOVE)) {
			Get()._MoveRenderer();
		}

		return 0;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

void ScalingWindow::_ResizeRenderer() noexcept {
	if (!_renderer->Resize()) {
		Logger::Get().Error("更改 Renderer 尺寸失败");
		return;
	}

	Render();
}

void ScalingWindow::_MoveRenderer() noexcept {
	_renderer->Move();
}

bool ScalingWindow::_CheckSrcState() noexcept {
	HWND hwndFore = GetForegroundWindow();

	if (hwndFore == Handle()) {
		// 缩放窗口不应该得到焦点，我们通过 WS_EX_NOACTIVATE 样式和处理 WM_MOUSEACTIVATE
		// 等消息来做到这一点。但如果由于某种我们尚未了解的机制这些手段都失败了，这里
		// 进行纠正。
		Win32Helper::SetForegroundWindow(_srcInfo.Handle());
		hwndFore = GetForegroundWindow();
	}

	// 在 3D 游戏模式下需检测前台窗口变化
	if (_options.Is3DGameMode() && !_CheckForegroundFor3DGameMode(hwndFore)) {
		return false;
	}

	RECT lastSrcWndRect = _srcInfo.WindowRect();

	if (!_srcInfo.UpdateState(hwndFore)) {
		return false;
	}

	if (_srcInfo.IsZoomed() && !_options.IsAllowScalingMaximized()) {
		return false;
	}

	if (_srcInfo.WindowRect() == lastSrcWndRect) {
		return true;
	} else {
		_isSrcRepositioning = true;
		return false;
	}
}

// 返回真表示应继续缩放
bool ScalingWindow::_CheckForegroundFor3DGameMode(HWND hwndFore) const noexcept {
	if (!hwndFore || hwndFore == _srcInfo.Handle()) {
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
	
	if (!IntersectRect(&rectForground, &rectForground, &_rendererRect)) {
		// 没有重叠
		return true;
	}

	// 允许稍微重叠，减少意外停止缩放的机率
	SIZE rectSize = Win32Helper::GetSizeOfRect(rectForground);
	return rectSize.cx < 8 || rectSize.cy < 8;
}

// 用于和其他程序交互
void ScalingWindow::_SetWindowProps() const noexcept {
	const HWND hWnd = Handle();
	SetProp(hWnd, L"Magpie.SrcHWND", _srcInfo.Handle());

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

LRESULT ScalingWindow::_BorderHelperWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_NCCREATE) {
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)((CREATESTRUCT*)lParam)->lpCreateParams);
	} else {
		switch (msg) {
#ifdef MP_DEBUG_BORDER
		case WM_ERASEBKGND:
		{
			// 用颜色标示辅助窗口
			int side = (int)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			HBRUSH hBrush = CreateSolidBrush(side % 2 == 0 ? RGB(255, 0, 0) : RGB(0, 0, 255));
			RECT clientRect;
			GetClientRect(hWnd, &clientRect);
			FillRect((HDC)wParam, &clientRect, hBrush);
			DeleteBrush(hBrush);
			return TRUE;
		}
#endif
		case WM_NCHITTEST:
		{
			POINT cursorPos{ GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam) };
			ScreenToClient(hWnd, &cursorPos);

			RECT titleBarClientRect;
			GetClientRect(hWnd, &titleBarClientRect);
			if (!PtInRect(&titleBarClientRect, cursorPos)) {
				return HTNOWHERE;
			}

			switch (GetWindowLongPtr(hWnd, GWLP_USERDATA)) {
			case 0:
			{
				const int resizeHandleWidth = titleBarClientRect.right - titleBarClientRect.left;
				if (cursorPos.y < 2 * resizeHandleWidth) {
					return HTTOPLEFT;
				} else if (cursorPos.y + 2 * resizeHandleWidth >= titleBarClientRect.bottom) {
					return HTBOTTOMLEFT;
				} else {
					return HTLEFT;
				}
			}
			case 1:
			{
				const int resizeHandleHeight = titleBarClientRect.bottom - titleBarClientRect.top;
				const int cornerLen = Get()._IsBorderless() ? resizeHandleHeight : 2 * resizeHandleHeight;
				if (cursorPos.x < cornerLen) {
					return HTTOPLEFT;
				} else if (cursorPos.x + cornerLen >= titleBarClientRect.right) {
					return HTTOPRIGHT;
				} else {
					return HTTOP;
				}
			}
			case 2:
			{
				const int resizeHandleWidth = titleBarClientRect.right - titleBarClientRect.left;
				if (cursorPos.y < 2 * resizeHandleWidth) {
					return HTTOPRIGHT;
				} else if (cursorPos.y + 2 * resizeHandleWidth >= titleBarClientRect.bottom) {
					return HTBOTTOMRIGHT;
				} else {
					return HTRIGHT;
				}
			}
			case 3:
			{
				const int resizeHandleHeight = titleBarClientRect.bottom - titleBarClientRect.top;
				if (cursorPos.x < resizeHandleHeight) {
					return HTBOTTOMLEFT;
				} else if (cursorPos.x + resizeHandleHeight >= titleBarClientRect.right) {
					return HTBOTTOMRIGHT;
				} else {
					return HTBOTTOM;
				}
			}
			}
			break;
		}
		case WM_NCLBUTTONDOWN:
		case WM_NCLBUTTONDBLCLK:
		case WM_NCMOUSEMOVE:
		case WM_NCLBUTTONUP:
		{
			if (wParam >= HTSIZEFIRST || wParam <= HTSIZELAST) {
				// 将这些消息传给主窗口才能调整窗口大小
				return Get()._MessageHandler(msg, wParam, lParam);
			}
			return 0;
		}
		}
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

void ScalingWindow::_CreateBorderHelperWindows() noexcept {
	static Ignore _ = [] {
		WNDCLASSEXW wcex{
			.cbSize = sizeof(wcex),
			.lpfnWndProc = _BorderHelperWndProc,
			.hInstance = wil::GetModuleInstanceHandle(),
			.lpszClassName = CommonSharedConstants::SCALING_BORDER_HELPER_WINDOW_CLASS_NAME
		};
		RegisterClassEx(&wcex);

		return Ignore();
	}();

	const bool isBorderless = _IsBorderless();
	for (int i = 0; i < 4; ++i) {
		if (!isBorderless && i != 1) {
			continue;
		}

		_hwndResizeHelpers[i].reset(CreateWindowEx(
#ifdef MP_DEBUG_BORDER
			WS_EX_NOACTIVATE,
#else
			WS_EX_NOACTIVATE | WS_EX_NOREDIRECTIONBITMAP,
#endif
			CommonSharedConstants::SCALING_BORDER_HELPER_WINDOW_CLASS_NAME,
			nullptr,
			WS_POPUP,
			0, 0, 0, 0,
			Handle(),
			NULL,
			wil::GetModuleInstanceHandle(),
			(void*)(intptr_t)i
		));
	}
}

void ScalingWindow::_RepostionBorderHelperWindows() noexcept {
	const int resizeHandleLen =
		GetSystemMetricsForDpi(SM_CXPADDEDBORDER, _currentDpi) +
		GetSystemMetricsForDpi(SM_CYSIZEFRAME, _currentDpi);
#ifdef MP_DEBUG_BORDER
	constexpr int flags = SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOCOPYBITS;
#else
	constexpr int flags = SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOREDRAW;
#endif

		// NoBorder 窗口 Win11 中四边都有客户区内的边框，边框上应可以调整窗口尺寸
	const RECT noBorderWindowRect{
		_windowRect.left + (LONG)_nonTopBorderThicknessInClient,
		_windowRect.top + (LONG)_topBorderThicknessInClient,
		_windowRect.right - (LONG)_nonTopBorderThicknessInClient,
		_windowRect.bottom - (LONG)_nonTopBorderThicknessInClient
	};

	// ┌───┬────────────┬───┐
	// │   │     1      │   │
	// │   ├────────────┤   │
	// │   │            │   │
	// │ 0 │            │ 2 │
	// │   │            │   │
	// │   ├────────────┤   │
	// │   │     3      │   │
	// └───┴────────────┴───┘
	if (const wil::unique_hwnd& hWnd = _hwndResizeHelpers[0]) {
		SetWindowPos(
			hWnd.get(),
			NULL,
			noBorderWindowRect.left - resizeHandleLen,
			noBorderWindowRect.top - resizeHandleLen,
			resizeHandleLen,
			noBorderWindowRect.bottom - noBorderWindowRect.top + 2 * resizeHandleLen,
			flags
		);
	}
	if (const wil::unique_hwnd& hWnd = _hwndResizeHelpers[1]) {
		SetWindowPos(
			hWnd.get(),
			NULL,
			noBorderWindowRect.left,
			noBorderWindowRect.top - resizeHandleLen,
			noBorderWindowRect.right - noBorderWindowRect.left,
			resizeHandleLen,
			flags
		);
	}
	if (const wil::unique_hwnd& hWnd = _hwndResizeHelpers[2]) {
		SetWindowPos(
			hWnd.get(),
			NULL,
			noBorderWindowRect.right,
			noBorderWindowRect.top - resizeHandleLen,
			resizeHandleLen,
			noBorderWindowRect.bottom - noBorderWindowRect.top + 2 * resizeHandleLen,
			flags
		);
	}
	if (const wil::unique_hwnd& hWnd = _hwndResizeHelpers[3]) {
		SetWindowPos(
			hWnd.get(),
			NULL,
			noBorderWindowRect.left,
			noBorderWindowRect.bottom,
			noBorderWindowRect.right - noBorderWindowRect.left,
			resizeHandleLen,
			flags
		);
	}
}

static LRESULT CALLBACK BkgWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_WINDOWPOSCHANGING) {
		// 确保始终在缩放窗口后
		((WINDOWPOS*)lParam)->hwndInsertAfter = ScalingWindow::Get().Handle();
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
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

	if (destRect.left > _rendererRect.left) {
		srcTouchRect.left -= lround((destRect.left - _rendererRect.left) / scaleX);
	}
	if (destRect.top > _rendererRect.top) {
		srcTouchRect.top -= lround((destRect.top - _rendererRect.top) / scaleX);
	}
	if (destRect.right < _rendererRect.right) {
		srcTouchRect.right += lround((_rendererRect.right - destRect.right) / scaleY);
	}
	if (destRect.bottom < _rendererRect.bottom) {
		srcTouchRect.bottom += lround((_rendererRect.bottom - destRect.bottom) / scaleY);
	}

	static Ignore _ = [] {
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

	SetProp(hWnd, L"Magpie.DestTouchLeft", (HANDLE)(INT_PTR)_rendererRect.left);
	SetProp(hWnd, L"Magpie.DestTouchTop", (HANDLE)(INT_PTR)_rendererRect.top);
	SetProp(hWnd, L"Magpie.DestTouchRight", (HANDLE)(INT_PTR)_rendererRect.right);
	SetProp(hWnd, L"Magpie.DestTouchBottom", (HANDLE)(INT_PTR)_rendererRect.bottom);
}

void ScalingWindow::_UpdateFrameMargins() const noexcept {
	assert(_options.IsWindowedMode());

	if (Win32Helper::GetOSVersion().IsWin11() || _IsBorderless()) {
		return;
	}

	// 由于缩放窗口有 WS_EX_NOREDIRECTIONBITMAP 样式，所以不需要绘制黑色实线。需要扩展到
	// 标题栏高度，原因见 XamlWindowT::_UpdateFrameMargins 中的注释。
	RECT frame{};
	AdjustWindowRectExForDpi(&frame, GetWindowStyle(Handle()), FALSE, 0, _currentDpi);

	MARGINS margins{ .cyTopHeight = -frame.top };
	DwmExtendFrameIntoClientArea(Handle(), &margins);
}

void ScalingWindow::_UpdateFocusState() const noexcept {
	if (_options.IsWindowedMode()) {
		// 根据源窗口状态绘制非客户区，我们必须自己控制非客户区是绘制成焦点状态还是非焦点
		// 状态，因为缩放窗口实际上永远不会得到焦点。
		DefWindowProc(Handle(), WM_NCACTIVATE, _srcInfo.IsFocused(), 0);

		if (_srcInfo.IsFocused() && !_options.IsDebugMode()) {
			// 置顶然后取消置顶使缩放窗口在最前面。有些窗口（如微信）使用单独的窗口实现假
			// 边框和阴影，缩放窗口应在它们前面。
			SetWindowPos(Handle(), HWND_TOPMOST,
				0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
			SetWindowPos(Handle(), HWND_NOTOPMOST,
				0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
		}

		return;
	}

	if (_options.IsDebugMode()) {
		return;
	}

	// 源窗口位于前台时将缩放窗口置顶，这使不支持 MPO 的显卡更容易激活 DirectFlip
	if (_srcInfo.IsFocused()) {
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

bool ScalingWindow::_IsBorderless() const noexcept {
	assert(_options.IsWindowedMode());

	const SrcWindowKind srcWindowKind = _srcInfo.WindowKind();
	// NoBorder: Win11 中这类窗口有着特殊的边框，因此和 Win10 的处理方式相同。
	// NoDecoration: Win11 中实现为无标题栏并隐藏边框。
	return srcWindowKind == SrcWindowKind::NoBorder || 
		(srcWindowKind == SrcWindowKind::NoDecoration && !Win32Helper::GetOSVersion().IsWin11());
}

// 返回缩放窗口跨越的屏幕数量，失败返回 0
ScalingError ScalingWindow::_CalcFullscreenRendererRect(uint32_t& monitorCount) noexcept {
	switch (_options.multiMonitorUsage) {
	// 使用距离源窗口最近的显示器
	case MultiMonitorUsage::Closest:
	{
		if (ScalingError error = _MoveSrcWindowIfNecessary(); error != ScalingError::NoError) {
			return error;
		}

		monitorCount = 1;
		return ScalingError::NoError;
	}
	// 使用源窗口跨越的所有显示器
	case MultiMonitorUsage::Intersected:
	{
		if (_srcInfo.IsZoomed()) {
			// 最大化的窗口不能跨越屏幕
			HMONITOR hMon = MonitorFromWindow(_srcInfo.Handle(), MONITOR_DEFAULTTONULL);
			MONITORINFO mi{ .cbSize = sizeof(mi) };
			if (!GetMonitorInfo(hMon, &mi)) {
				Logger::Get().Win32Error("GetMonitorInfo 失败");
				return ScalingError::ScalingFailedGeneral;
			}

			_rendererRect = mi.rcMonitor;
			monitorCount = 1;
			return ScalingError::NoError;
		}

		// [0] 存储源窗口坐标，[1] 存储计算结果
		struct MonitorEnumParam {
			RECT srcRect;
			RECT destRect;
			uint32_t monitorCount;
		} param{};

		// 使用窗口框架矩形来计算和哪些屏幕相交，而不是被缩放区域
		if (!Win32Helper::GetWindowFrameRect(_srcInfo.Handle(), param.srcRect)) {
			Logger::Get().Error("GetWindowFrameRect 失败");
			return ScalingError::ScalingFailedGeneral;
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
			return ScalingError::ScalingFailedGeneral;
		}

		_rendererRect = param.destRect;

		if (ScalingError error = _MoveSrcWindowIfNecessary(); error != ScalingError::NoError) {
			return error;
		}

		monitorCount = param.monitorCount;
		return ScalingError::NoError;
	}
	// 使用所有显示器 (Virtual Screen)
	case MultiMonitorUsage::All:
	{
		int vsWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
		int vsHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
		int vsX = GetSystemMetrics(SM_XVIRTUALSCREEN);
		int vsY = GetSystemMetrics(SM_YVIRTUALSCREEN);
		_rendererRect = { vsX, vsY, vsX + vsWidth, vsY + vsHeight };

		if (ScalingError error = _MoveSrcWindowIfNecessary(); error != ScalingError::NoError) {
			return error;
		}

		monitorCount = GetSystemMetrics(SM_CMONITORS);
		return ScalingError::NoError;
	}
	default:
		assert(false);
		return ScalingError::ScalingFailedGeneral;
	}
}

ScalingError ScalingWindow::_MoveSrcWindowIfNecessary() noexcept {
	HMONITOR hMonitor = MonitorFromWindow(_srcInfo.Handle(), MONITOR_DEFAULTTONULL);
	assert(hMonitor);

	MONITORINFO mi{ .cbSize = sizeof(mi) };
	if (!GetMonitorInfo(hMonitor, &mi)) {
		Logger::Get().Win32Error("GetMonitorInfo 失败");
		return ScalingError::ScalingFailedGeneral;
	}

	if (_options.multiMonitorUsage == MultiMonitorUsage::Closest) {
		_rendererRect = mi.rcMonitor;
	}

	if (_srcInfo.IsZoomed()) {
		return ScalingError::NoError;
	}

	const RECT& srcRect = _srcInfo.SrcRect();
	const SIZE srcSize = Win32Helper::GetSizeOfRect(srcRect);
	const SIZE monitorSize = Win32Helper::GetSizeOfRect(mi.rcMonitor);

	if (srcSize.cx > monitorSize.cx || srcSize.cy > monitorSize.cy) {
		// 被缩放区域比屏幕更大时无法使用 DesktopDuplication 捕获
		if (_options.captureMethod == CaptureMethod::DesktopDuplication) {
			return ScalingError::CaptureFailed;
		} else {
			return ScalingError::NoError;
		}
	}

	// 作为优化，如果窗口有一部分不在屏幕上则将被缩放区域移动到屏幕中央。Desktop Duplication
	// 不能捕获屏幕外的内容，所以被缩放区域必须在屏幕内。
	// 
	// 无需考虑被任务栏遮挡，缩放时任务栏将自动隐藏。
	bool shouldMove = false;
	if (_options.captureMethod == CaptureMethod::DesktopDuplication) {
		shouldMove = !PtInRect(&mi.rcMonitor, POINT{ srcRect.left,srcRect.top }) 
			|| !PtInRect(&mi.rcMonitor, POINT{ srcRect.left,srcRect.bottom })
			|| !PtInRect(&mi.rcMonitor, POINT{ srcRect.right,srcRect.top })
			|| !PtInRect(&mi.rcMonitor, POINT{ srcRect.right,srcRect.bottom });
	} else {
		shouldMove = !MonitorFromPoint(POINT{ srcRect.left,srcRect.top }, MONITOR_DEFAULTTONULL)
			|| !MonitorFromPoint(POINT{ srcRect.left,srcRect.bottom }, MONITOR_DEFAULTTONULL)
			|| !MonitorFromPoint(POINT{ srcRect.right,srcRect.top }, MONITOR_DEFAULTTONULL)
			|| !MonitorFromPoint(POINT{ srcRect.right,srcRect.bottom }, MONITOR_DEFAULTTONULL);
	}

	if (shouldMove) {
		// 不要跨屏幕移动，否则如果 DPI 缩放不同会造成源窗口尺寸改变
		int offsetX = mi.rcMonitor.left + (monitorSize.cx - srcSize.cx) / 2 - srcRect.left;
		int offsetY = mi.rcMonitor.top + (monitorSize.cy - srcSize.cy) / 2 - srcRect.top;
		_MoveSrcWindow(offsetX, offsetY);
	}

	return ScalingError::NoError;
}

void ScalingWindow::_MoveSrcWindow(int offsetX, int offsetY) noexcept {
	const RECT& srcWindowRect = _srcInfo.WindowRect();
	SetWindowPos(
		_srcInfo.Handle(),
		NULL,
		srcWindowRect.left + offsetX,
		srcWindowRect.top + offsetY,
		0,
		0,
		SWP_NOACTIVATE | SWP_NOREDRAW | SWP_NOSIZE | SWP_NOZORDER
	);
	_srcInfo.UpdateAfterMoved(offsetX, offsetY);
}

}
