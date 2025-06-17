#pragma once

namespace Magpie {

class CursorManager {
public:
	CursorManager() = default;
	CursorManager(const CursorManager&) = delete;
	CursorManager(CursorManager&&) = delete;

	~CursorManager() noexcept;

	void Update() noexcept;

	void OnScalingWindowPosChanged() noexcept;

	void OnSrcStartMove() noexcept;

	void OnSrcEndMove() noexcept;

	// 光标不在缩放窗口上或隐藏时为 NULL
	HCURSOR CursorHandle() const noexcept {
		return _hCursor;
	}

	// 屏幕坐标
	POINT CursorPos() const noexcept {
		return _cursorPos;
	}

	bool IsCursorCaptured() const noexcept {
		return _isUnderCapture;
	}

	bool IsCursorCapturedOnForeground() const noexcept {
		return _isCapturedOnForeground;
	}

	bool IsCursorOnOverlay() const noexcept {
		return _isOnOverlay;
	}
	void IsCursorOnOverlay(bool value) noexcept;

	bool IsCursorCapturedOnOverlay() const noexcept {
		return _isCapturedOnOverlay;
	}
	void IsCursorCapturedOnOverlay(bool value) noexcept;

	const std::atomic<int16_t>& SrcBorderHitTest() const noexcept {
		return _srcBorderHitTest;
	}

private:
	void _ShowSystemCursor(bool show, bool onDestory = false);

	void _AdjustCursorSpeed() noexcept;

	void _RestoreCursorSpeed() noexcept;

	void _ReliableSetCursorPos(POINT pos) const noexcept;

	winrt::fire_and_forget _UpdateCursorClip() noexcept;

	void _UpdateCursorPos() noexcept;

	void _StartCapture(POINT& cursorPos) noexcept;

	bool _StopCapture(POINT& cursorPos, bool onDestroy = false) noexcept;

	void _SetClipCursor(const RECT& clipRect, bool is3DGameMode = false) noexcept;

	void _RestoreClipCursor() noexcept;

	HCURSOR _hCursor = NULL;
	POINT _cursorPos { std::numeric_limits<LONG>::max(),std::numeric_limits<LONG>::max() };

	RECT _lastClip{ std::numeric_limits<LONG>::max() };
	RECT _lastRealClip{ std::numeric_limits<LONG>::max() };

	int _originCursorSpeed = 0;

	std::chrono::steady_clock::time_point _sizeCursorStartTime{};

	// HTTRANSPARENT 表示正在进行命中测试
	std::atomic<int16_t> _srcBorderHitTest = 0;

	bool _isUnderCapture = false;
	// 当缩放后的光标位置在交换链窗口上且没有被其他窗口挡住时应绘制光标
	bool _shouldDrawCursor = false;
	
	bool _isCapturedOnForeground = false;

	bool _isOnOverlay = false;
	bool _isCapturedOnOverlay = false;

	bool _isSystemCursorShown = true;

	bool _isWaitingForHitTest = false;
	bool _shouldUpdateCursorClip = false;

	static inline const HCURSOR _hDiagonalSize1Cursor = LoadCursor(NULL, IDC_SIZENWSE);
	static inline const HCURSOR _hDiagonalSize2Cursor = LoadCursor(NULL, IDC_SIZENESW);
	static inline const HCURSOR _hHorizontalSizeCursor = LoadCursor(NULL, IDC_SIZEWE);
	static inline const HCURSOR _hVerticalSizeCursor = LoadCursor(NULL, IDC_SIZENS);
};

}
