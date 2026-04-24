#pragma once
#include "ImGuiBackend.h"

namespace Magpie {

class ImGuiImpl {
public:
	ImGuiImpl() = default;
	ImGuiImpl(const ImGuiImpl&) = delete;
	ImGuiImpl(ImGuiImpl&&) = delete;

	~ImGuiImpl() noexcept;

	bool Initialize(D3D12Context& d3d12Context) noexcept;

private:
	ImGuiBackend _backend;
};

}
