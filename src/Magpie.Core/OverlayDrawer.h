#pragma once
#include "ImGuiImpl.h"

namespace Magpie {

struct OverlayOptions;

class OverlayDrawer {
public:
	OverlayDrawer() = default;
	OverlayDrawer(const OverlayDrawer&) = delete;
	OverlayDrawer(OverlayDrawer&&) = delete;

	bool Initialize(D3D12Context& d3d12Context, OverlayOptions& overlayOptions) noexcept;

private:
	D3D12Context* _d3d12Context = nullptr;
	OverlayOptions* _overlayOptions = nullptr;

	ImGuiImpl _imguiImpl;

	float _dpiScale = 1.0f;

	struct {
		std::string gpuName;
	} _hardwareInfo;
};

}
