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

	bool Initialize(
		D3D12Context& d3d12Context,
		OverlayOptions& overlayOptions,
		const RECT& rendererRect,
		const RECT& destRect,
		const ColorInfo& colorInfo
	) noexcept;

	bool NeedRedraw() const noexcept {
		return _hasInput;
	}

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

	void OnColorInfoChanged(const ColorInfo& colorInfo) noexcept;

	void MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

private:
	bool _BuildFonts() noexcept;

	bool _AnyVisibleWindow() const noexcept;

	bool _DrawToolbar(uint32_t fps, POINT cursorPos, int& itemId) noexcept;

	float _CalcToolbarAlpha(POINT cursorPos) const noexcept;

	D3D12Context* _d3d12Context = nullptr;
	OverlayOptions* _overlayOptions = nullptr;

	RECT _destRect{};

	ImGuiImpl _imguiImpl;

	float _dpiScale = 1.0f;

	uint32_t _lastFPS = std::numeric_limits<uint32_t>::max();
	float _lastToolbarAlpha = -1.0f;

	ImFont* _iconFont = nullptr;

	struct {
		std::string gpuName;
	} _hardwareInfo;

	bool _isMoving = false;
	bool _isResizing = false;
	bool _isToolbarVisible = true;
	bool _isProfilerVisible = false;
#ifdef _DEBUG
	bool _isDemoWindowVisible = false;
#endif
	bool _hasInput = false;
	bool _isToolbarItemActive = false;
	bool _isToolbarPinned = false;
	bool _isCursorOnCaptionArea = false;
};

}
