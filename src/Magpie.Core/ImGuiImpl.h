#pragma once
#include "ImGuiBackend.h"
#include "ScalingOptions.h"
#include <parallel_hashmap/phmap.h>

namespace Magpie {

class GraphicsContext;

class ImGuiImpl {
public:
	ImGuiImpl() = default;
	ImGuiImpl(const ImGuiImpl&) = delete;
	ImGuiImpl(ImGuiImpl&&) = delete;

	~ImGuiImpl() noexcept;

	bool Initialize(D3D12Context& d3d12Context) noexcept;

	void NewFrame(
		POINT cursorPos,
		phmap::flat_hash_map<std::string, OverlayWindowOption>& windowOptions,
		float fittsLawAdjustment,
		float dpiScale
	) noexcept;

	HRESULT Draw(
		GraphicsContext& graphicsContext,
		uint64_t frameFenceValue,
		uint64_t completedFenceValue
	) noexcept;

	void OnResizingChanged(bool value) noexcept;

	void OnResized(const RECT& rendererRect, const RECT& destRect) noexcept;

	void OnMovingChanged(bool value) noexcept;

	void OnMoved(const RECT& rendererRect, const RECT& destRect) noexcept;

	void OnCursorCapturedOnForegroundChanged(bool value) noexcept;

private:
	void _UpdateMousePos(POINT cursorPos, float fittsLawAdjustment) const noexcept;

	ImGuiBackend _backend;
	phmap::flat_hash_map<std::string, ImVec4> _windowRects;

	RECT _rendererRect{};
	RECT _destRect{};

	bool _isMoving = false;
	bool _isResizing = false;
	bool _isCursorCapturedOnForeground = false;
};

}
