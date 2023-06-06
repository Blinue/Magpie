#include "pch.h"
#include "ScalingWindow.h"
#include "CommonSharedConstants.h"
#include "Logger.h"

namespace Magpie::Core {

bool ScalingWindow::Create(HINSTANCE hInstance) noexcept {
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

	if (!Handle()) {
		return false;
	}

	// 设置窗口不透明
	// 不完全透明时可关闭 DirectFlip
	if (!SetLayeredWindowAttributes(Handle(), 0, 255, LWA_ALPHA)) {
		Logger::Get().Win32Error("SetLayeredWindowAttributes 失败");
	}

	ShowWindow(Handle(), SW_SHOWMAXIMIZED);

	return true;
}

void ScalingWindow::Render() noexcept {
	
}

}
