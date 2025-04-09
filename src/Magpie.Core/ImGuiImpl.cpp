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
#include <ranges>
#include "StrHelper.h"

namespace Magpie {

static bool operator==(const ImVec2& l, const ImVec2& r) noexcept {
	return l.x == r.x && l.y == r.y;
}

static bool operator==(const ImVec4& l, const ImVec4& r) noexcept {
	return l.x == r.x && l.y == r.y && l.z == r.z && l.w == r.w;
}

static const char* GetWindowIDFromName(const char* name) noexcept {
	size_t idPos = std::string_view(name).find("##");
	if (idPos == std::string_view::npos) {
		return name;
	} else {
		return name + idPos + 2;
	}
}

ImGuiImpl::~ImGuiImpl() noexcept {
	if (ImGui::GetCurrentContext()) {
		ImGui::DestroyContext();
	}
}

bool ImGuiImpl::Initialize(DeviceResources& deviceResources) noexcept {
#ifdef _DEBUG
	// 检查 ImGUI 版本是否匹配
	if (!IMGUI_CHECKVERSION()) {
		Logger::Get().Error("ImGui 的头文件与链接库版本不同");
		return false;
	}
#endif

	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.BackendPlatformName = "Magpie";
	io.ConfigFlags |= ImGuiConfigFlags_NavNoCaptureKeyboard | ImGuiConfigFlags_NoMouseCursorChange;
	// 禁用 ini 配置文件
	io.IniFilename = nullptr;

	if (!_backend.Initialize(deviceResources)) {
		Logger::Get().Error("初始化 ImGuiBackend 失败");
		return false;
	}

	return true;
}

bool ImGuiImpl::BuildFonts() noexcept {
	return _backend.BuildFonts();
}

void ImGuiImpl::NewFrame(
	phmap::flat_hash_map<std::string, OverlayWindowOption>& windowOptions,
	float fittsLawAdjustment,
	float dpiScale
) noexcept {
	ImGuiIO& io = ImGui::GetIO();

	{
		const SIZE destSize = Win32Helper::GetSizeOfRect(ScalingWindow::Get().Renderer().DestRect());
		ImVec2 newDisplaySize((float)destSize.cx, (float)destSize.cy);
		if (io.DisplaySize != newDisplaySize) {
			io.DisplaySize = newDisplaySize;
			// 调整缩放窗口尺寸时强制调整叠加层窗口位置
			_windowRects.clear();
		}
	}

	_UpdateMousePos(fittsLawAdjustment);

	// 不接受键盘输入
	if (io.WantCaptureKeyboard) {
		io.AddKeyEvent(ImGuiKey_Enter, true);
		io.AddKeyEvent(ImGuiKey_Enter, false);
	}

	ImGui::NewFrame();
	
	for (ImGuiWindow* window : ImGui::GetCurrentContext()->Windows) {
		if (window->Flags & (ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoMove)) {
			continue;
		}

		// 排除 Debug##Default 窗口和尚未初始化完成的窗口
		if (window->IsFallbackWindow || window->Appearing) {
			continue;
		}

		ImVec2 pos = window->Pos;

		// 将窗口限制在视口内
		if (io.DisplaySize.x > window->Size.x) {
			pos.x = std::clamp(pos.x, 0.0f, io.DisplaySize.x - window->Size.x);
		} else {
			pos.x = 0;
		}

		if (io.DisplaySize.y > window->Size.y) {
			pos.y = std::clamp(pos.y, 0.0f, io.DisplaySize.y - window->Size.y);
		} else {
			pos.y = 0;
		}

		const char* windowId = GetWindowIDFromName(window->Name);
		if (auto it = windowOptions.find(windowId); it != windowOptions.end()) {
			OverlayWindowOption& option = it->second;

			auto it1 = _windowRects.find(windowId);
			if (it1 == _windowRects.end()) {
				// 第一次显示或调整缩放窗口大小时叠加层窗口应根据规则调整位置

				if (option.hArea == 0) {
					pos.x = option.hPos * dpiScale;
				} else if (option.hArea == 1) {
					pos.x = io.DisplaySize.x * option.hPos - window->Size.x / 2;
				} else if (option.hArea == 2) {
					pos.x = io.DisplaySize.x - option.hPos * dpiScale - window->Size.x;
				} else {
					assert(false);
				}

				if (option.vArea == 0) {
					pos.y = option.vPos * dpiScale;
				} else if (option.vArea == 1) {
					pos.y = io.DisplaySize.y * option.vPos - window->Size.y / 2;
				} else if (option.vArea == 2) {
					pos.y = io.DisplaySize.y - option.vPos * dpiScale - window->Size.y;
				} else {
					assert(false);
				}

				// 再次将窗口限制在视口内
				if (io.DisplaySize.x > window->Size.x) {
					pos.x = std::clamp(pos.x, 0.0f, io.DisplaySize.x - window->Size.x);
				} else {
					pos.x = 0;
				}

				if (io.DisplaySize.y > window->Size.y) {
					pos.y = std::clamp(pos.y, 0.0f, io.DisplaySize.y - window->Size.y);
				} else {
					pos.y = 0;
				}
			} else if (it1->second != ImVec4(pos.x, pos.y, window->Size.x, window->Size.y)) {
				// 当且仅当用户移动窗口或调整窗口大小后后重新计算贴靠的边，调整缩放窗口大小时应保持
				// 贴靠的边不变。我们根据两侧边距的比例决定贴靠哪边或者都不贴靠。

				// 这些阈值决定是否贴靠在某一边上，它们不是定值，而是窗口尺寸和画面尺寸的比例。这个
				// 算法的效果出乎意料的好，因为窗口两侧边距较大时人对比例更敏感，较小时则对差值更敏
				// 感。
				const float thresholdX = std::max(window->Size.x / io.DisplaySize.x, 0.2f);
				const float thresholdY = std::max(window->Size.y / io.DisplaySize.y, 0.2f);

				// 根据左右边距比例决定贴靠
				float ratio = pos.x / (io.DisplaySize.x - pos.x - window->Size.x);
				if (ratio < thresholdX) {
					option.hArea = 0;
					option.hPos = pos.x / dpiScale;
				} else if (ratio <= 1 / thresholdX) {
					option.hArea = 1;
					option.hPos = (pos.x + window->Size.x / 2) / io.DisplaySize.x;
				} else {
					option.hArea = 2;
					option.hPos = (io.DisplaySize.x - pos.x - window->Size.x) / dpiScale;
				}
				
				// 根据上下边距比例决定贴靠
				ratio = pos.y / (io.DisplaySize.y - pos.y - window->Size.y);
				if (ratio < thresholdY) {
					option.vArea = 0;
					option.vPos = pos.y / dpiScale;
				} else if (ratio <= 1 / thresholdY) {
					option.vArea = 1;
					option.vPos = (pos.y + window->Size.y / 2) / io.DisplaySize.y;
				} else {
					option.vArea = 2;
					option.vPos = (io.DisplaySize.y - pos.y - window->Size.y) / dpiScale;
				}
			}

			ImGui::SetWindowPos(window, pos);

			// 此时 window->Pos 已更新，记录新的窗口位置
			_windowRects[windowId] = ImVec4(window->Pos.x, window->Pos.y, window->Size.x, window->Size.y);
		} else {
			ImGui::SetWindowPos(window, pos);
		}
	}

	// 调整缩放窗口大小或鼠标被前台窗口捕获时避免鼠标跳跃
	CursorManager& cursorManager = ScalingWindow::Get().CursorManager();
	if (!ScalingWindow::Get().IsResizingOrMoving() && !cursorManager.IsCursorCapturedOnForeground()) {
		cursorManager.IsCursorOnOverlay(io.WantCaptureMouse);
	}
}

void ImGuiImpl::Draw(POINT drawOffset) noexcept {
	ImGui::Render();

	const RECT& rendererRect = ScalingWindow::Get().RendererRect();
	const RECT& destRect = ScalingWindow::Get().Renderer().DestRect();
	const POINT viewportOffset = {
		destRect.left - rendererRect.left + drawOffset.x,
		destRect.top - rendererRect.top + drawOffset.y
	};
	_backend.RenderDrawData(*ImGui::GetDrawData(), viewportOffset);
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

void ImGuiImpl::_UpdateMousePos(float fittsLawAdjustment) noexcept {
	ImGuiIO& io = ImGui::GetIO();
	io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);

	// 调整缩放窗口大小或鼠标被前台窗口捕获时不应和叠加层交互
	const CursorManager& cursorManager = ScalingWindow::Get().CursorManager();
	if (ScalingWindow::Get().IsResizingOrMoving() || cursorManager.IsCursorCapturedOnForeground()) {
		return;
	}

	const POINT cursorPos = cursorManager.CursorPos();

	// 转换为目标矩形局部坐标
	const RECT& destRect = ScalingWindow::Get().Renderer().DestRect();
	io.MousePos.x = float(cursorPos.x - destRect.left);
	io.MousePos.y = float(cursorPos.y - destRect.top);

	// 下移鼠标的逻辑位置使得在上边缘可以选中工具栏按钮
	if (io.MousePos.y >= 0 && io.MousePos.y < fittsLawAdjustment) {
		io.MousePos.y = fittsLawAdjustment;
	}
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

std::optional<ImVec4> ImGuiImpl::GetWindowRect(const char* id) const noexcept {
	const std::string suffix = StrHelper::Concat("##", id);
	for (ImGuiWindow* window : ImGui::GetCurrentContext()->Windows) {
		if (std::string_view(window->Name).ends_with(suffix)) {
			return ImVec4(
				window->Pos.x,
				window->Pos.y,
				window->Pos.x + window->Size.x,
				window->Pos.y + window->Size.y
			);
		}
	}

	return std::nullopt;
}

const char* ImGuiImpl::GetHoveredWindowId() const noexcept {
	const ImVec2 mousePos = ImGui::GetIO().MousePos;
	// 自顶向下遍历
	for (ImGuiWindow* window : ImGui::GetCurrentContext()->Windows | std::views::reverse) {
		if (window->IsFallbackWindow || window->Hidden) {
			continue;
		}

		if (window->Rect().Contains(mousePos)) {
			return GetWindowIDFromName(window->Name);
		}
	}

	return nullptr;
}

}
