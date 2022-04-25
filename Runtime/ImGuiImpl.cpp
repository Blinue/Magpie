#include "pch.h"
#include "ImGuiImpl.h"
#include <imgui.h>
#include <imgui_internal.h>
#include "imgui_impl_dx11.h"
#include "App.h"
#include "CursorManager.h"
#include "DeviceResources.h"
#include "Renderer.h"
#include "Logger.h"
#include "WindowsMessages.h"
#include "Config.h"


ImGuiImpl::~ImGuiImpl() {
	ImGuiIO& io = ImGui::GetIO();
	io.BackendPlatformName = nullptr;
	io.BackendPlatformUserData = nullptr;

	App::Get().UnregisterWndProcHandler(_handlerId);

	if (_hHookThread) {
		PostThreadMessage(_hookThreadId, WM_QUIT, 0, 0);
		WaitForSingleObject(_hHookThread, 1000);
	}

	ImGui_ImplDX11_Shutdown();
	ImGui::DestroyContext();
}

static std::optional<LRESULT> ImGui_ImplMagpie_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	ImGuiIO& io = ImGui::GetIO();

	if (msg == WindowsMessages::WM_MYMOUSEWHEEL) {
		if (io.WantCaptureMouse) {
			io.MouseWheel += GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
		}
		return 0;
	}

	if (!io.WantCaptureMouse) {
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
			App::Get().GetCursorManager().OnCursorCapturedOnOverlay();
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
			App::Get().GetCursorManager().OnCursorReleasedOnOverlay();
		}
		
		break;
	}
	case WM_MOUSEWHEEL:
		if (App::Get().GetConfig().IsBreakpointMode()) {
			io.MouseWheel += (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
		}
		break;
	}

	return std::nullopt;
}

bool ImGuiImpl::Initialize() {
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
	io.ImeWindowHandle = App::Get().GetHwndHost();
	io.ConfigFlags |= ImGuiConfigFlags_NavNoCaptureKeyboard | ImGuiConfigFlags_NoMouseCursorChange;

	auto& dr = App::Get().GetDeviceResources();
	ImGui_ImplDX11_Init(dr.GetD3DDevice(), dr.GetD3DDC());

	if (!dr.GetRenderTargetView(dr.GetBackBuffer(), &_rtv)) {
		return false;
	}

	_handlerId = App::Get().RegisterWndProcHandler(ImGui_ImplMagpie_WndProcHandler);
	if (_handlerId == 0) {
		return false;
	}

	return true;
}

static void ImGui_ImplMagpie_UpdateMousePos() {
	ImGuiIO& io = ImGui::GetIO();

	POINT pos;
	CursorManager& cm = App::Get().GetCursorManager();
	if (cm.HasCursor()) {
		pos = *cm.GetCursorPos();
	} else {
		GetCursorPos(&pos);

		if (WindowFromPoint(pos) != App::Get().GetHwndHost()) {
			io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
			return;
		}

		const RECT& hostRect = App::Get().GetHostWndRect();
		pos.x -= hostRect.left;
		pos.y -= hostRect.top;
	}

	const RECT& outputRect = App::Get().GetRenderer().GetOutputRect();
	pos.x -= outputRect.left;
	pos.y -= outputRect.top;

	io.MousePos = ImVec2((float)pos.x, (float)pos.y);
}

void ImGuiImpl::NewFrame() {
	ImGuiIO& io = ImGui::GetIO();

	// Setup display size (every frame to accommodate for window resizing)
	const RECT& hostRect = App::Get().GetHostWndRect();
	const RECT& outputRect = App::Get().GetRenderer().GetOutputRect();
	io.DisplaySize = ImVec2((float)(hostRect.right - outputRect.left), (float)(hostRect.bottom - outputRect.top));

	// Update OS mouse position
	ImGui_ImplMagpie_UpdateMousePos();

	// 不接受键盘输入
	if (io.WantCaptureKeyboard) {
		io.AddKeyEvent(ImGuiKey_Enter, true);
		io.AddKeyEvent(ImGuiKey_Enter, false);
	}

	bool originWantCaptureMouse = io.WantCaptureMouse;

	ImGui_ImplDX11_NewFrame();
	ImGui::NewFrame();

	CursorManager& cm = App::Get().GetCursorManager();

	if (io.WantCaptureMouse) {
		if (!originWantCaptureMouse) {
			cm.OnCursorHoverOverlay();
		}
	} else {
		if (originWantCaptureMouse) {
			cm.OnCursorLeaveOverlay();
		}
	}

	// 将所有 ImGUI 窗口限制在视口内
	SIZE outputSize = Utils::GetSizeOfRect(App::Get().GetRenderer().GetOutputRect());
	for (ImGuiWindow* window : ImGui::GetCurrentContext()->Windows) {
		if (outputSize.cx > window->Size.x) {
			window->Pos.x = std::clamp(window->Pos.x, 0.0f, outputSize.cx - window->Size.x);
		} else {
			window->Pos.x = 0;
		}

		if (outputSize.cy > window->Size.y) {
			window->Pos.y = std::clamp(window->Pos.y, 0.0f, outputSize.cy - window->Size.y);
		} else {
			window->Pos.y = 0;
		}
	}
}

static LRESULT CALLBACK LowLevelMouseProc(
  _In_ int    nCode,
  _In_ WPARAM wParam,
  _In_ LPARAM lParam
) {
	if (nCode != HC_ACTION || wParam != WM_MOUSEWHEEL || !ImGui::GetIO().WantCaptureMouse) {
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	}

	auto window = ImGui::GetCurrentContext()->HoveredWindow;
	if (!window || window->ScrollMax.y == 0.0f || window->Collapsed) {
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	}

	PostMessage(App::Get().GetHwndHost(), WindowsMessages::WM_MYMOUSEWHEEL, ((MSLLHOOKSTRUCT*)lParam)->mouseData, 0);

	// 阻断滚轮消息，防止传给源窗口
	return -1;
}

static DWORD WINAPI ThreadProc(LPVOID lpThreadParameter) {
	HHOOK hook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, NULL, 0);

	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	UnhookWindowsHookEx(hook);
	return 0;
}

void ImGuiImpl::EndFrame() {
	const RECT& outputRect = App::Get().GetRenderer().GetOutputRect();
	ImGui::GetDrawData()->DisplayPos = ImVec2(float(-outputRect.left), float(-outputRect.top));

	auto d3dDC = App::Get().GetDeviceResources().GetD3DDC();
	d3dDC->OMSetRenderTargets(1, &_rtv, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	if (!_hHookThread && !App::Get().GetConfig().IsBreakpointMode()) {
		auto window = ImGui::GetCurrentContext()->HoveredWindow;
		if (window && window->ScrollMax.y > 0.0f && !window->Collapsed) {
			_hHookThread = CreateThread(nullptr, 0, ThreadProc, nullptr, 0, &_hookThreadId);
		}
	}
}

void ImGuiImpl::ClearStates() {
	ImGuiIO& io = ImGui::GetIO();
	io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
	std::fill(std::begin(io.MouseDown), std::end(io.MouseDown), false);

	auto& cm = App::Get().GetCursorManager();
	if (cm.IsCursorCapturedOnOverlay()) {
		if (GetCapture() == App::Get().GetHwndHost()) {
			ReleaseCapture();
		}
		cm.OnCursorReleasedOnOverlay();
	}

	if (cm.IsCursorOnOverlay()) {
		cm.OnCursorLeaveOverlay();
	}

	// 更新状态
	ImGui::NewFrame();
	ImGui::EndFrame();

	if (io.WantCaptureMouse) {
		// 拖拽时隐藏 UI 需渲染两帧才能重置 WantCaptureMouse
		ImGui::NewFrame();
		ImGui::EndFrame();
	}
}
