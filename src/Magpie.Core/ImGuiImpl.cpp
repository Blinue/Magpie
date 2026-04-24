#include "pch.h"
#include "ImGuiImpl.h"
#include "Logger.h"
#include <imgui.h>

namespace Magpie {

static bool operator==(const ImVec2& l, const ImVec2& r) noexcept {
	return l.x == r.x && l.y == r.y;
}

static bool operator==(const ImVec4& l, const ImVec4& r) noexcept {
	return l.x == r.x && l.y == r.y && l.z == r.z && l.w == r.w;
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

}
