#pragma once

namespace Magpie {

class D3D12Context;

class ImGuiBackend {
public:
	ImGuiBackend() = default;
	ImGuiBackend(const ImGuiBackend&) = delete;
	ImGuiBackend(ImGuiBackend&&) = delete;

	bool Initialize(D3D12Context& d3d12Context) noexcept;

private:
	D3D12Context* _d3d12Context = nullptr;
};

}
