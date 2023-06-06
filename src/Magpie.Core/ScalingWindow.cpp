#include "pch.h"
#include "ScalingWindow.h"
#include "CommonSharedConstants.h"
#include "Logger.h"
#include "Renderer.h"

namespace Magpie::Core {

ScalingWindow::ScalingWindow() noexcept {}

ScalingWindow::~ScalingWindow() noexcept {}

bool ScalingWindow::Create(HINSTANCE hInstance, ScalingOptions&& options) noexcept {
	if (_hWnd) {
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
		WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_LAYERED | WS_EX_NOREDIRECTIONBITMAP | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
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
	if (!_renderer->Initialize(_hWnd, options)) {
		return false;
	}

	ShowWindow(_hWnd, SW_SHOWMAXIMIZED);

	return true;
}

void ScalingWindow::Render() noexcept {
	_renderer->Render();
}

LRESULT ScalingWindow::_MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
	switch (msg) {
	case WM_DESTROY:
	{
		_renderer.reset();
		break;
	}
	}
	return base_type::_MessageHandler(msg, wParam, lParam);
}

}
