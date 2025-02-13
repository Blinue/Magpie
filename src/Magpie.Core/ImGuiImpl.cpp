#include "pch.h"
#include "ImGuiImpl.h"
#include <imgui.h>
#include <imgui_internal.h>
#include "ImGuiBackend.h"
#include "CursorManager.h"
#include "DeviceResources.h"
#include "Renderer.h"
#include "Logger.h"
#include "Win32Helper.h"
#include "ScalingWindow.h"
#include "CursorManager.h"

namespace Magpie {

ImGuiImpl::~ImGuiImpl() noexcept {
	if (ImGui::GetCurrentContext()) {
		ImGui::DestroyContext();
	}
}

bool ImGuiImpl::Initialize(DeviceResources* deviceResources) noexcept {
#ifdef _DEBUG
	// 检查 ImGUI 版本是否匹配
	if (!IMGUI_CHECKVERSION()) {
		Logger::Get().Error("ImGui 的头文件与链接库版本不同");
		return false;
	}
#endif

	ImGui::CreateContext();

	// Setup backend capabilities flags
	ImGuiIO& io = ImGui::GetIO();
	io.BackendPlatformName = "Magpie";
	io.ConfigFlags |= ImGuiConfigFlags_NavNoCaptureKeyboard | ImGuiConfigFlags_NoMouseCursorChange;

	if (!_backend.Initialize(deviceResources)) {
		Logger::Get().Error("初始化 ImGuiBackend 失败");
		return false;
	}

	return true;
}

bool ImGuiImpl::BuildFonts() noexcept {
	return _backend.BuildFonts();
}

void ImGuiImpl::NewFrame() noexcept {
	ImGuiIO& io = ImGui::GetIO();

	// Setup display size (every frame to accommodate for window resizing)
	const SIZE outputSize = Win32Helper::GetSizeOfRect(ScalingWindow::Get().Renderer().DestRect());
	io.DisplaySize = ImVec2((float)outputSize.cx, (float)outputSize.cy);

	// Update OS mouse position
	_UpdateMousePos();

	// 不接受键盘输入
	if (io.WantCaptureKeyboard) {
		io.AddKeyEvent(ImGuiKey_Enter, true);
		io.AddKeyEvent(ImGuiKey_Enter, false);
	}

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

	ScalingWindow::Get().CursorManager().IsCursorOnOverlay(io.WantCaptureMouse);
}

void ImGuiImpl::Draw() noexcept {
	const RECT& swapChainRect = ScalingWindow::Get().SwapChainRect();
	const RECT& destRect = ScalingWindow::Get().Renderer().DestRect();

	ImGui::Render();
	ImDrawData& drawData = *ImGui::GetDrawData();
	drawData.DisplayPos = ImVec2(
		float(swapChainRect.left - destRect.left),
		float(swapChainRect.top - destRect.top)
	);
	drawData.DisplaySize = ImVec2(
		float(destRect.right - swapChainRect.left),
		float(destRect.bottom - swapChainRect.top)
	);

	_backend.RenderDrawData(drawData);
}

void ImGuiImpl::Tooltip(const char* content, float maxWidth) noexcept {
	ImVec2 padding = ImGui::GetStyle().WindowPadding;
	ImVec2 contentSize = ImGui::CalcTextSize(content, nullptr, false, maxWidth - 2 * padding.x);
	ImVec2 windowSize(contentSize.x + 2 * padding.x, contentSize.y + 2 * padding.y);
	ImGui::SetNextWindowSize(windowSize);

	ImVec2 windowPos = ImGui::GetMousePos();
	windowPos.x += 16 * ImGui::GetStyle().MouseCursorScale;
	windowPos.y += 8 * ImGui::GetStyle().MouseCursorScale;

	SIZE outputSize = Win32Helper::GetSizeOfRect(ScalingWindow::Get().Renderer().DestRect());
	windowPos.x = std::clamp(windowPos.x, 0.0f, outputSize.cx - windowSize.x);
	windowPos.y = std::clamp(windowPos.y, 0.0f, outputSize.cy - windowSize.y);

	ImGui::SetNextWindowPos(windowPos);

	ImGui::SetNextWindowBgAlpha(ImGui::GetStyle().Colors[ImGuiCol_PopupBg].w);
	ImGui::Begin("tooltip", NULL, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing);

	ImGui::PushTextWrapPos(maxWidth - padding.x);
	ImGui::TextUnformatted(content);
	ImGui::PopTextWrapPos();

	ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
	ImGui::End();
}

void ImGuiImpl::_UpdateMousePos() noexcept {
	ImGuiIO& io = ImGui::GetIO();
	io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);

	const CursorManager& cursorManager = ScalingWindow::Get().CursorManager();

	if (cursorManager.IsCursorCapturedOnForeground()) {
		// 光标被前台窗口捕获时应避免造成光标跳跃
		return;
	}

	const POINT cursorPos = cursorManager.CursorPos();
	if (cursorPos.x == std::numeric_limits<LONG>::max()) {
		// 无光标
		return;
	}
	
	const RECT& swapChainRect = ScalingWindow::Get().SwapChainRect();
	const RECT& destRect = ScalingWindow::Get().Renderer().DestRect();

	io.MousePos.x = float(cursorPos.x + swapChainRect.left - destRect.left);
	io.MousePos.y = float(cursorPos.y + swapChainRect.top - destRect.top);
}

void ImGuiImpl::ClearStates() noexcept {
	ImGuiIO& io = ImGui::GetIO();
	io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
	std::fill(std::begin(io.MouseDown), std::end(io.MouseDown), false);

	CursorManager& cursorManager = ScalingWindow::Get().CursorManager();
	cursorManager.IsCursorCapturedOnOverlay(false);
	cursorManager.IsCursorOnOverlay(false);

	// 更新状态
	ImGui::NewFrame();
	ImGui::EndFrame();

	if (io.WantCaptureMouse) {
		// 拖拽时隐藏 UI 需渲染两帧才能重置 WantCaptureMouse
		ImGui::NewFrame();
		ImGui::EndFrame();
	}
}

void ImGuiImpl::MessageHandler(UINT msg, WPARAM wParam, LPARAM /*lParam*/) noexcept {
	ImGuiIO& io = ImGui::GetIO();

	if (!io.WantCaptureMouse) {
		// 3D 游戏模式下显示叠加层会使缩放窗口不透明，这时点击非叠加层区域应关闭叠加层
		if (msg == WM_LBUTTONDOWN && ScalingWindow::Get().Options().Is3DGameMode()) {
			ScalingWindow::Get().Renderer().SetOverlayVisibility(false);
		}
		return;
	}

	// 缩放窗口不会收到双击消息
	switch (msg) {
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	{
		if (!ImGui::IsAnyMouseDown()) {
			ScalingWindow::Get().CursorManager().IsCursorCapturedOnOverlay(true);
		}
		
		io.MouseDown[msg == WM_LBUTTONDOWN ? 0 : 1] = true;
		break;
	}
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	{
		io.MouseDown[msg == WM_LBUTTONUP ? 0 : 1] = false;

		if (!ImGui::IsAnyMouseDown()) {
			ScalingWindow::Get().CursorManager().IsCursorCapturedOnOverlay(false);
		}

		break;
	}
	case WM_MOUSEWHEEL:
	{
		io.MouseWheel += (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
		break;
	}
	case WM_MOUSEHWHEEL:
	{
		io.MouseWheelH += (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
		break;
	}
	}
}

}
