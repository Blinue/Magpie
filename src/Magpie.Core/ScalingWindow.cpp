#include "pch.h"
#include "ScalingWindow.h"
#include "CommonSharedConstants.h"
#include "Logger.h"
#include "Renderer.h"
#include "Win32Utils.h"
#include "WindowHelper.h"

namespace Magpie::Core {

ScalingWindow::ScalingWindow() noexcept {}

ScalingWindow::~ScalingWindow() noexcept {}

bool ScalingWindow::Create(HINSTANCE hInstance, HWND hwndSrc, ScalingOptions&& options) noexcept {
	if (_hWnd) {
		return false;
	}

	_hwndSrc = hwndSrc;
	// 缩放结束后才失效
	_options = std::move(options);

	if (!GetWindowRect(hwndSrc, &_srcWndRect)) {
		return false;
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
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
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

	_renderer = std::make_unique<Renderer>();
	if (!_renderer->Initialize(hwndSrc, _hWnd, _options)) {
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

	_renderer->Render();
}

LRESULT ScalingWindow::_MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
	switch (msg) {
	case WM_DESTROY:
	{
		_renderer.reset();
		_options = {};
		_hwndSrc = NULL;
		_srcWndRect = {};
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

	// 如果捕获模式可以捕获到弹窗，则允许小的弹窗
	/*if (MagApp::Get().GetFrameSource().IsScreenCapture()
		&& GetWindowStyle(hwndForeground) & (WS_POPUP | WS_CHILD)
		) {
		if (!Win32Utils::GetWindowFrameRect(hwndForeground, rectForground)) {
			Logger::Get().Error("GetWindowFrameRect 失败");
			return false;
		}

		// 弹窗如果完全在源窗口客户区内则不退出全屏
		const RECT& srcFrameRect = MagApp::Get().GetFrameSource().GetSrcFrameRect();
		if (rectForground.left >= srcFrameRect.left
			&& rectForground.right <= srcFrameRect.right
			&& rectForground.top >= srcFrameRect.top
			&& rectForground.bottom <= srcFrameRect.bottom
		) {
			return true;
		}
	}*/

	if (rectForground == RECT{}) {
		HRESULT hr = DwmGetWindowAttribute(hwndForeground,
			DWMWA_EXTENDED_FRAME_BOUNDS, &rectForground, sizeof(rectForground));
		if (FAILED(hr)) {
			Logger::Get().ComError("DwmGetWindowAttribute 失败", hr);
			return false;
		}
	}

	RECT scalingWndRect;
	GetWindowRect(_hWnd, &scalingWndRect);
	IntersectRect(&rectForground, &scalingWndRect, &rectForground);

	// 允许稍微重叠，否则前台窗口最大化时会意外退出
	return rectForground.right - rectForground.left < 10 || rectForground.right - rectForground.top < 10;
}

}
