#pragma once

namespace Magpie::Core {

class CursorManager {
public:
	CursorManager() = default;
	CursorManager(const CursorManager&) = delete;
	CursorManager(CursorManager&&) = delete;

	~CursorManager() noexcept;

	bool Initialize() noexcept;

	void Update() noexcept;

	HCURSOR Cursor() const noexcept {
		return _hCursor;
	}

	// 缩放窗口局部坐标
	POINT CursorPos() const noexcept {
		return _cursorPos;
	}

	bool IsCursorOnOverlay() const noexcept {
		return _isOnOverlay;
	}
	void IsCursorOnOverlay(bool value) noexcept;

	bool IsCursorCapturedOnOverlay() const noexcept {
		return _isCapturedOnOverlay;
	}
	void IsCursorCapturedOnOverlay(bool value) noexcept;

private:
	void _ShowSystemCursor(bool show, bool onDestory = false);

	void _AdjustCursorSpeed() noexcept;

	void _UpdateCursorClip() noexcept;

	void _StartCapture(POINT& cursorPos) noexcept;

	bool _StopCapture(POINT& cursorPos, bool onDestroy = false) noexcept;

	HCURSOR _hCursor = NULL;
	POINT _cursorPos { std::numeric_limits<LONG>::max(),std::numeric_limits<LONG>::max() };

	int _originCursorSpeed = 0;

	bool _isUnderCapture = false;
	// 当缩放后的光标位置在缩放窗口上且没有被其他窗口挡住时应绘制光标
	bool _shouldDrawCursor = false;

	bool _isOnOverlay = false;
	bool _isCapturedOnOverlay = false;

	bool _isSystemCursorShown = true;
};

}
