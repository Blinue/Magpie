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

	bool Initialize(
		D3D12Context& d3d12Context,
		const RECT& rendererRect,
		const RECT& destRect,
		const ColorInfo& colorInfo
	) noexcept;

	void PrepareNewFrame(
		POINT cursorPos,
		std::string_view fittsLawWindowId,
		float fittsLawAdjustment
	) noexcept;

	void NewFrame(
		phmap::flat_hash_map<std::string, OverlayWindowOption>& windowOptions,
		float dpiScale
	) noexcept;

	HRESULT Draw(
		GraphicsContext& graphicsContext,
		uint64_t frameFenceValue,
		uint64_t completedFenceValue
	) noexcept;

	wil::zstring_view GetHoveredWindowId() const noexcept {
		return _hoveredWindowId;
	}

	void OnResizingChanged(bool value) noexcept;

	void OnResized(const RECT& rendererRect, const RECT& destRect) noexcept;

	void OnMovingChanged(bool value) noexcept;

	void OnMoved(const RECT& rendererRect, const RECT& destRect) noexcept;

	void OnCursorCapturedOnForegroundChanged(bool value) noexcept;

	void OnColorInfoChanged(const ColorInfo& colorInfo) noexcept;

	bool MessageHandler(UINT msg, WPARAM wParam) noexcept;

	std::optional<ImVec4> GetWindowRect(const char* id) const noexcept;

private:
	ImGuiBackend _backend;
	phmap::flat_hash_map<std::string, ImVec4> _windowRects;

	RECT _rendererRect{};
	RECT _destRect{};

	wil::zstring_view _hoveredWindowId;

	bool _isMoving = false;
	bool _isResizing = false;
	bool _isCursorCapturedOnForeground = false;
	bool _isCursorOnOverlay = false;
};

}
