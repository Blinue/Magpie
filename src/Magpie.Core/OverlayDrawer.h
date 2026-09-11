#pragma once
#include "ImGuiImpl.h"

namespace Magpie {

struct OverlayOptions;
class GraphicsContext;

class OverlayDrawer {
public:
	OverlayDrawer() = default;
	OverlayDrawer(const OverlayDrawer&) = delete;
	OverlayDrawer(OverlayDrawer&&) = delete;

	bool Initialize(D3D12Context& d3d12Context, OverlayOptions& overlayOptions) noexcept;

	void OnResizingChanged(bool value) noexcept;

	void OnResized(const RECT& rendererRect, const RECT& destRect) noexcept;

	void OnMovingChanged(bool value) noexcept;

	void OnMoved(const RECT& rendererRect, const RECT& destRect) noexcept;

	void OnCursorCapturedOnForegroundChanged(bool value) noexcept;

	HRESULT Draw(
		GraphicsContext& graphicsContext,
		POINT cursorPos,
		uint32_t fps,
		uint64_t frameFenceValue,
		uint64_t completedFenceValue
	) noexcept;

private:
	bool _AnyVisibleWindow() const noexcept;

	D3D12Context* _d3d12Context = nullptr;
	OverlayOptions* _overlayOptions = nullptr;

	ImGuiImpl _imguiImpl;

	float _dpiScale = 1.0f;

	struct {
		std::string gpuName;
	} _hardwareInfo;

	bool _isToolbarVisible = false;
	bool _isProfilerVisible = false;
#ifdef _DEBUG
	bool _isDemoWindowVisible = false;
#endif
};

}
