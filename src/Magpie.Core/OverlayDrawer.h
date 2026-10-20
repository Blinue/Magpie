#pragma once
#include "ImGuiImpl.h"

namespace Magpie {

struct OverlayOptions;
class GraphicsContext;
class FrameProducer;

class OverlayDrawer {
public:
	OverlayDrawer() = default;
	OverlayDrawer(const OverlayDrawer&) = delete;
	OverlayDrawer(OverlayDrawer&&) = delete;

	bool Initialize(
		D3D12Context& d3d12Context,
		OverlayOptions& overlayOptions,
		FrameProducer& frameProducer,
		const RECT& rendererRect,
		const RECT& destRect,
		const ColorInfo& colorInfo
	) noexcept;

	bool NeedRedraw() const noexcept {
		return _hasMouseInput;
	}

	HRESULT Draw(
		GraphicsContext& graphicsContext,
		bool isCursorOnRenderer,
		POINT cursorPos,
		uint32_t fps,
		uint64_t completedFenceValue,
		uint64_t frameFenceValue,
		bool& hasDrawed
	) noexcept;

	ToolbarState GetToolbarState() const noexcept;

	HRESULT SetToolbarState(ToolbarState value) noexcept;

	void OnResizingChanged(bool value) noexcept;

	void OnResized(const RECT& rendererRect, const RECT& destRect) noexcept;

	void OnMovingChanged(bool value) noexcept;

	void OnMoved(const RECT& rendererRect, const RECT& destRect) noexcept;

	void OnCursorCapturedOnForegroundChanged(bool value) noexcept;

	void OnColorInfoChanged(const ColorInfo& colorInfo) noexcept;

	void MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

private:
	HRESULT _InitializeImpl() noexcept;

	HRESULT _BuildFonts() noexcept;

	bool _AnyVisibleWindow() const noexcept;

	bool _DrawToolbar(uint32_t fps, POINT cursorPos) noexcept;

	float _CalcToolbarAlpha(POINT cursorPos) const noexcept;

	D3D12Context* _d3d12Context = nullptr;
	OverlayOptions* _overlayOptions = nullptr;
	FrameProducer* _frameProducer = nullptr;

	RECT _rendererRect{};
	RECT _destRect{};

	winrt::AdvancedColorKind _acKind = winrt::AdvancedColorKind::StandardDynamicRange;
	float _sdrWhiteLevel = 1.0f;

	ImGuiImpl _imguiImpl;

	float _dpiScale = 1.0f;

	//uint32_t _lastFPS = std::numeric_limits<uint32_t>::max();
	float _lastToolbarAlpha = -1.0f;

	ImFont* _iconFont = nullptr;
	ImFont* _fontMonoNumbers = nullptr;

	struct {
		std::string gpuName;
	} _hardwareInfo;

	bool _isImplInitialized = false;
	bool _isMoving = false;
	bool _isResizing = false;
	bool _isToolbarVisible = false;
	bool _isProfilerVisible = false;
#ifdef _DEBUG
	bool _isDemoWindowVisible = false;
#endif
	bool _hasMouseInput = false;
	bool _isToolbarItemActive = false;
	bool _isToolbarPinned = false;
	bool _isCursorOnCaptionArea = false;
};

}
