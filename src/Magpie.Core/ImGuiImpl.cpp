#include "pch.h"
#include "ImGuiImpl.h"
#include <imgui.h>
#include <imgui_internal.h>
#include "ImGuiBackend.h"
#include "CursorManager.h"
#include "DeviceResources.h"
#include "Renderer.h"
#include "Logger.h"
#include "Win32Utils.h"
#include "ScalingWindow.h"
#include "CursorManager.h"

namespace Magpie::Core {

ImGuiImpl::~ImGuiImpl() noexcept {
	/*MagApp::Get().UnregisterWndProcHandler(_handlerId);

	if (_hHookThread) {
		PostThreadMessage(_hookThreadId, WM_QUIT, 0, 0);
		WaitForSingleObject(_hHookThread, 1000);
	}*/

	if (ImGui::GetCurrentContext()) {
		ImGui::DestroyContext();
	}
}
/*
static std::optional<LRESULT> WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	ImGuiIO& io = ImGui::GetIO();

	if (!io.WantCaptureMouse) {
		if (msg == WM_LBUTTONDOWN && MagApp::Get().GetOptions().Is3DGameMode()) {
			MagApp::Get().GetRenderer().SetUIVisibility(false);
		}
		return std::nullopt;
	}

	switch (msg) {
	case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
	case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK:
	{
		int button = 0;
		if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK) { button = 0; }
		if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK) { button = 1; }
		if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONDBLCLK) { button = 2; }
		if (msg == WM_XBUTTONDOWN || msg == WM_XBUTTONDBLCLK) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4; }

		if (!ImGui::IsAnyMouseDown()) {
			if (!GetCapture()) {
				SetCapture(hwnd);
			}
			MagApp::Get().GetCursorManager().OnCursorCapturedOnOverlay();
		}

		io.MouseDown[button] = true;
		break;
	}
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	case WM_XBUTTONUP:
	{
		int button = 0;
		if (msg == WM_LBUTTONUP) { button = 0; }
		if (msg == WM_RBUTTONUP) { button = 1; }
		if (msg == WM_MBUTTONUP) { button = 2; }
		if (msg == WM_XBUTTONUP) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4; }

		io.MouseDown[button] = false;

		if (!ImGui::IsAnyMouseDown()) {
			if (GetCapture() == hwnd) {
				ReleaseCapture();
			}
			MagApp::Get().GetCursorManager().OnCursorReleasedOnOverlay();
		}

		break;
	}
	case WM_MOUSEWHEEL:
		io.MouseWheel += (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
		break;
	case WM_MOUSEHWHEEL:
		io.MouseWheelH += (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
		break;
	}

	return std::nullopt;
}

static LRESULT CALLBACK LowLevelMouseProc(
	_In_ int    nCode,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam
) {
	if (nCode != HC_ACTION || !ImGui::GetIO().WantCaptureMouse) {
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	}

	if (wParam == WM_MOUSEWHEEL || wParam == WM_MOUSEHWHEEL) {
		// 向主线程发送滚动数据
		// 使用 Windows 消息进行线程同步
		PostMessage(MagApp::Get().GetHwndHost(), (UINT)wParam, ((MSLLHOOKSTRUCT*)lParam)->mouseData, 0);

		// 阻断滚轮消息，防止传给源窗口
		return -1;
	} else if (wParam >= WM_LBUTTONDOWN && wParam <= WM_RBUTTONUP) {
		PostMessage(MagApp::Get().GetHwndHost(), (UINT)wParam, 0, 0);

		// 阻断点击消息，防止传给源窗口
		return -1;
	} else {
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	}
}

static DWORD WINAPI ThreadProc(LPVOID lpThreadParameter) {
	HHOOK hook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, NULL, 0);
	if (!hook) {
		Logger::Get().Win32Error("注册鼠标钩子失败");
		return 1;
	}

	Logger::Get().Info("已注册鼠标钩子");

	// 鼠标钩子需要消息循环
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	UnhookWindowsHookEx(hook);
	Logger::Get().Info("已销毁鼠标钩子");
	return 0;
}
*/
bool ImGuiImpl::Initialize(DeviceResources* deviceResources) noexcept {
#ifdef _DEBUG
	// 检查 ImGUI 版本是否匹配
	if (!IMGUI_CHECKVERSION()) {
		Logger::Get().Error("ImGui 的头文件与链接库版本不同");
		return false;
	}
#endif // _DEBUG

	ImGui::CreateContext();

	// Setup backend capabilities flags
	ImGuiIO& io = ImGui::GetIO();
	io.BackendPlatformUserData = nullptr;
	io.BackendPlatformName = "Magpie";
	io.ImeWindowHandle = ScalingWindow::Get().Handle();
	io.ConfigFlags |= ImGuiConfigFlags_NavNoCaptureKeyboard | ImGuiConfigFlags_NoMouseCursorChange;

	if (!_backend.Initialize(deviceResources)) {
		Logger::Get().Error("初始化 ImGuiBackend 失败");
		return false;
	}

	/*_handlerId = MagApp::Get().RegisterWndProcHandler(WndProcHandler);

	// 断点模式下不注册鼠标钩子，否则调试时鼠标无法使用
	if (!MagApp::Get().GetOptions().IsDebugMode() && !MagApp::Get().GetOptions().Is3DGameMode()) {
		_hHookThread = CreateThread(nullptr, 0, ThreadProc, nullptr, 0, &_hookThreadId);
		if (!_hHookThread) {
			Logger::Get().Win32Error("创建线程失败");
		}
	}*/

	return true;
}

void ImGuiImpl::BeginFrame() {
	ImGuiIO& io = ImGui::GetIO();

	// Setup display size (every frame to accommodate for window resizing)
	const SIZE outputSize = Win32Utils::GetSizeOfRect(ScalingWindow::Get().Renderer().DestRect());
	io.DisplaySize = ImVec2((float)outputSize.cx, (float)outputSize.cy);

	// Update OS mouse position
	_UpdateMousePos();

	// 不接受键盘输入
	if (io.WantCaptureKeyboard) {
		io.AddKeyEvent(ImGuiKey_Enter, true);
		io.AddKeyEvent(ImGuiKey_Enter, false);
	}

	//bool originWantCaptureMouse = io.WantCaptureMouse;

	_backend.BeginFrame();
	ImGui::NewFrame();

	// 将所有 ImGUI 窗口限制在视口内
	for (ImGuiWindow* window : ImGui::GetCurrentContext()->Windows) {
		if (window->Flags & ImGuiWindowFlags_Tooltip) {
			continue;
		}

		ImVec2 pos = window->Pos;

		if (outputSize.cx > window->Size.x) {
			pos.x = std::clamp(pos.x, 0.0f, outputSize.cx - window->Size.x);
		} else {
			pos.x = 0;
		}

		if (outputSize.cy > window->Size.y) {
			pos.y = std::clamp(pos.y, 0.0f, outputSize.cy - window->Size.y);
		} else {
			pos.y = 0;
		}

		ImGui::SetWindowPos(window, pos);
	}

	/*CursorManager& cm = MagApp::Get().GetCursorManager();

	if (io.WantCaptureMouse) {
		if (!originWantCaptureMouse) {
			cm.OnCursorHoverOverlay();
		}
	} else {
		if (originWantCaptureMouse) {
			cm.OnCursorLeaveOverlay();
		}
	}*/
}

void ImGuiImpl::EndFrame() {
	ImGui::Render();

	const RECT& scalingRect = ScalingWindow::Get().WndRect();
	const RECT& destRect = ScalingWindow::Get().Renderer().DestRect();

	ImGui::GetDrawData()->DisplayPos = ImVec2(
		float(scalingRect.left - destRect.left),
		float(scalingRect.top - destRect.top)
	);
	ImGui::GetDrawData()->DisplaySize = ImVec2(
		float(destRect.right - scalingRect.left),
		float(destRect.bottom - scalingRect.top)
	);

	_backend.RenderDrawData(ImGui::GetDrawData());
}

void ImGuiImpl::Tooltip(const char* /*content*/, float /*maxWidth*/) {
	/*ImVec2 padding = ImGui::GetStyle().WindowPadding;
	ImVec2 contentSize = ImGui::CalcTextSize(content, nullptr, false, maxWidth - 2 * padding.x);
	ImVec2 windowSize(contentSize.x + 2 * padding.x, contentSize.y + 2 * padding.y);
	ImGui::SetNextWindowSize(windowSize);

	ImVec2 windowPos = ImGui::GetMousePos();
	windowPos.x += 16 * ImGui::GetStyle().MouseCursorScale;
	windowPos.y += 8 * ImGui::GetStyle().MouseCursorScale;

	SIZE outputSize = Win32Utils::GetSizeOfRect(MagApp::Get().GetRenderer().GetOutputRect());
	windowPos.x = std::clamp(windowPos.x, 0.0f, outputSize.cx - windowSize.x);
	windowPos.y = std::clamp(windowPos.y, 0.0f, outputSize.cy - windowSize.y);

	ImGui::SetNextWindowPos(windowPos);

	ImGui::SetNextWindowBgAlpha(ImGui::GetStyle().Colors[ImGuiCol_PopupBg].w);
	ImGui::Begin("tooltip", NULL, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing);

	ImGui::PushTextWrapPos(maxWidth - padding.x);
	ImGui::TextUnformatted(content);
	ImGui::PopTextWrapPos();

	ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
	ImGui::End();*/
}

void ImGuiImpl::_UpdateMousePos() noexcept {
	ImGuiIO& io = ImGui::GetIO();

	/*if (ScalingWindow::Get().Options().Is3DGameMode() && !MagApp::Get().GetRenderer().IsUIVisiable()) {
		io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
		return;
	}

	POINT pos;
	CursorManager& cm = MagApp::Get().GetCursorManager();
	if (cm.HasCursor()) {
		pos = *cm.GetCursorPos();
	} else {
		GetCursorPos(&pos);

		if (WindowFromPoint(pos) != MagApp::Get().GetHwndHost()) {
			io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
			return;
		}

		const RECT& hostRect = MagApp::Get().GetHostWndRect();
		pos.x -= hostRect.left;
		pos.y -= hostRect.top;
	}*/

	const CursorManager& cursorManager = ScalingWindow::Get().CursorManager();
	if (!cursorManager.Cursor()) {
		io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
		return;
	}

	const POINT cursorPos = cursorManager.CursorPos();
	const RECT& scalingRect = ScalingWindow::Get().WndRect();
	const RECT& destRect = ScalingWindow::Get().Renderer().DestRect();

	io.MousePos = ImVec2(
		float(cursorPos.x + scalingRect.left - destRect.left),
		float(cursorPos.y + scalingRect.top - destRect.top)
	);
}

void ImGuiImpl::ClearStates() {
	/*ImGuiIO& io = ImGui::GetIO();
	io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
	std::fill(std::begin(io.MouseDown), std::end(io.MouseDown), false);

	auto& cm = ScalingWindow::Get().CursorManager();
	if (cm.IsCursorCapturedOnOverlay()) {
		if (GetCapture() == MagApp::Get().GetHwndHost()) {
			ReleaseCapture();
		}
		cm.OnCursorReleasedOnOverlay();
	}

	if (cm.IsCursorOnOverlay()) {
		cm.OnCursorLeaveOverlay();
	}

	// 更新状态
	ImGui::BeginFrame();
	ImGui::EndFrame();

	if (io.WantCaptureMouse) {
		// 拖拽时隐藏 UI 需渲染两帧才能重置 WantCaptureMouse
		ImGui::BeginFrame();
		ImGui::EndFrame();
	}*/
}

}
