#include "pch.h"
#include "ImGuiImpl.h"
#include "Logger.h"
#include "ScalingWindow.h"
#include <imgui.h>
#include <imgui_internal.h>

namespace Magpie {

static bool operator==(const ImVec2& l, const ImVec2& r) noexcept {
	return l.x == r.x && l.y == r.y;
}

static bool operator==(const ImVec4& l, const ImVec4& r) noexcept {
	return l.x == r.x && l.y == r.y && l.z == r.z && l.w == r.w;
}

static std::string_view GetWindowIDFromName(std::string_view name) noexcept {
	size_t idPos = name.find("##");
	if (idPos != std::string_view::npos) {
		name.remove_prefix(idPos + 2);
	}
	return name;
}

ImGuiImpl::~ImGuiImpl() noexcept {
	if (ImGui::GetCurrentContext()) {
		ImGui::DestroyContext();
	}
}

bool ImGuiImpl::Initialize(D3D12Context& d3d12Context) noexcept {
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
	io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
	io.ConfigNavCaptureKeyboard = false;
	// 禁用 ini 配置文件
	io.IniFilename = nullptr;
#ifndef _DEBUG
	// Release 配置下禁用重复 ID 检查
	io.ConfigDebugHighlightIdConflicts = false;
#endif

	if (!_backend.Initialize(d3d12Context)) {
		Logger::Get().Error("ImGuiBackend::Initialize 失败");
		return false;
	}

	return true;
}

void ImGuiImpl::NewFrame(
	POINT cursorPos,
	phmap::flat_hash_map<std::string, OverlayWindowOption>& windowOptions,
	float fittsLawAdjustment,
	float dpiScale
) noexcept {
	ImGuiIO& io = ImGui::GetIO();

	{
		ImVec2 newDisplaySize(
			float(_destRect.right - _destRect.left),
			float(_destRect.bottom - _destRect.top)
		);
		if (io.DisplaySize != newDisplaySize) {
			io.DisplaySize = newDisplaySize;
			// 调整缩放窗口尺寸时强制调整叠加层窗口位置
			_windowRects.clear();
		}
	}

	_UpdateMousePos(cursorPos, fittsLawAdjustment);

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

		std::string_view windowId = GetWindowIDFromName(window->Name);
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
	if (!_isResizing && !_isMoving && !_isCursorCapturedOnForeground) {
		ScalingWindow::Get().OnCursorOnOverlayChanged(io.WantCaptureMouse);
	}
}

HRESULT ImGuiImpl::Draw(
	GraphicsContext& graphicsContext,
	uint64_t frameFenceValue,
	uint64_t completedFenceValue
) noexcept {
	ImGui::Render();

	POINT viewportOffset = {
		_destRect.left - _rendererRect.left,
		_destRect.top - _rendererRect.top
	};
	HRESULT hr = _backend.RenderDrawData(*ImGui::GetDrawData(), viewportOffset,
		graphicsContext, frameFenceValue, completedFenceValue);
	if (FAILED(hr)) {
		Logger::Get().ComError("ImGuiBackend::RenderDrawData 失败", hr);
		return hr;
	}

	return S_OK;
}

void ImGuiImpl::OnResizingChanged(bool value) noexcept {
	_isResizing = value;
}

void ImGuiImpl::OnResized(const RECT& rendererRect, const RECT& destRect) noexcept {
	_rendererRect = rendererRect;
	_destRect = destRect;
}

void ImGuiImpl::OnMovingChanged(bool value) noexcept {
	_isMoving = value;
}

void ImGuiImpl::OnMoved(const RECT& rendererRect, const RECT& destRect) noexcept {
	OnResized(rendererRect, destRect);
}

void ImGuiImpl::OnCursorCapturedOnForegroundChanged(bool value) noexcept {
	_isCursorCapturedOnForeground = value;
}

void ImGuiImpl::_UpdateMousePos(POINT cursorPos, float fittsLawAdjustment) const noexcept {
	ImGuiIO& io = ImGui::GetIO();
	io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
	
	// 调整缩放窗口大小或鼠标被前台窗口捕获时不应和叠加层交互
	if (_isResizing || _isMoving || _isCursorCapturedOnForeground) {
		return;
	}

	// 转换为目标矩形局部坐标
	io.MousePos.x = float(cursorPos.x - _destRect.left);
	io.MousePos.y = float(cursorPos.y - _destRect.top);

	// 下移鼠标的逻辑位置使得在上边缘可以选中工具栏按钮
	if (io.MousePos.y >= 0 && io.MousePos.y < fittsLawAdjustment) {
		io.MousePos.y = fittsLawAdjustment;
	}
}

}
