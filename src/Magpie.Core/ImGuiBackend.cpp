#include "pch.h"
#include "ImGuiBackend.h"
#include <imgui.h>

namespace Magpie {

bool ImGuiBackend::Initialize(D3D12Context& d3d12Context) noexcept {
	_d3d12Context = &d3d12Context;

	ImGuiIO& io = ImGui::GetIO();
	io.BackendRendererName = "Magpie";
	// 支持 ImDrawCmd::VtxOffset
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasTextures;

	return true;
}

}
