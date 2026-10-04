#pragma once

namespace Magpie {

class CursorManager {
public:
	CursorManager() = default;
	CursorManager(const CursorManager&) = delete;
	CursorManager(CursorManager&&) = delete;

	void Initialize(
		const RECT& srcRect,
		const RECT& rendererRect,
		const RECT& destRect,
		bool isSrcMoving,
		bool isSrcFocused
	) noexcept;

	~CursorManager() noexcept;

	std::pair<HCURSOR, POINT> Update() noexcept;

	void OnResizingChanged(bool value) noexcept;

	void OnResized(const RECT& rendererRect, const RECT& destRect) noexcept;

	void OnMovingChanged(bool value) noexcept;

	void OnMoved(const RECT& rendererRect, const RECT& destRect) noexcept;

	void OnSrcMovingChanged(bool value) noexcept;

	void OnSrcMoved(const RECT& srcRect) noexcept;

	void OnSrcFocusChanged(bool focused) noexcept;

	void OnCursorOnOverlayChanged(bool value) noexcept;

	bool IsCursorCapturedOnOverlay() const noexcept {
		return _isCapturedOnOverlay;
	}
	void IsCursorCapturedOnOverlay(bool value) noexcept;

	int16_t GetSrcHitTest() const noexcept {
		return _lastCompletedHitTestResult;
	}

private:
	POINT _SrcToScaling(POINT pt, bool skipBorder) const noexcept;

	enum class _RoundMethod {
		Round,
		Floor,
		Ceil
	};

	POINT _ScalingToSrc(POINT pt, _RoundMethod roundType = _RoundMethod::Round) const noexcept;

	void _ShowSystemCursor(bool show, bool onDestory = false);

	void _AdjustCursorSpeed() noexcept;

	void _RestoreCursorSpeed() noexcept;

	void _ReliableSetCursorPos(POINT pos) const noexcept;

	winrt::fire_and_forget _SrcHitTestAsync(POINT screenPos) noexcept;

	void _ClearHitTestResult() noexcept;

	void _UpdateCursorState() noexcept;

	void _ClipCursorForMonitors(POINT cursorPos) noexcept;

	void _ClipCursorOnSrcMoving() noexcept;

	void _UpdateCursorPos() noexcept;

	void _StartVirtualization(POINT& cursorPos) noexcept;

	bool _StopVirtualization(POINT& cursorPos, bool onDestroy = false) noexcept;

	void _SetClipCursor(const RECT& clipRect, bool is3DGameMode = false) noexcept;

	void _RestoreClipCursor() noexcept;

	RECT _srcRect{};
	RECT _rendererRect{};
	RECT _destRect{};

	HCURSOR _hCursor = NULL;
	POINT _cursorPos{ std::numeric_limits<LONG>::max() };

	// 用于确保拖拽源窗口和缩放窗口时光标位置稳定，使用相对于渲染矩形的局部坐标
	POINT _localCursorPosOnMoving{ std::numeric_limits<LONG>::max() };

	// 用于防止光标移动到边框的过程中闪烁
	std::chrono::steady_clock::time_point _sizeCursorStartTime{};

	RECT _lastClip{ std::numeric_limits<LONG>::max() };
	RECT _lastRealClip{ std::numeric_limits<LONG>::max() };

	int _originCursorSpeed = 0;

	uint32_t _nextHitTestId = 0;
	uint32_t _lastCompletedHitTestId = 0;
	POINT _lastCompletedHitTestPos{ std::numeric_limits<LONG>::max() };
	int16_t _lastCompletedHitTestResult = HTNOWHERE;

	bool _isMoving = false;
	bool _isResizing = false;
	bool _isSrcMoving = false;
	bool _isSrcFocused = false;

	bool _isVirtualized = false;
	// 当缩放后的光标位置在渲染矩形内且没有被其他窗口挡住时应绘制光标
	bool _shouldDrawCursor = false;
	
	bool _isCapturedOnForeground = false;

	bool _isOnOverlay = false;
	bool _isCapturedOnOverlay = false;

	bool _isSystemCursorShown = true;

	static inline const HCURSOR _hDiagonalSize1Cursor = LoadCursor(NULL, IDC_SIZENWSE);
	static inline const HCURSOR _hDiagonalSize2Cursor = LoadCursor(NULL, IDC_SIZENESW);
	static inline const HCURSOR _hHorizontalSizeCursor = LoadCursor(NULL, IDC_SIZEWE);
	static inline const HCURSOR _hVerticalSizeCursor = LoadCursor(NULL, IDC_SIZENS);
};

}
