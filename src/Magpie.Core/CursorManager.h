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

private:
	void _ShowSystemCursor(bool show);

	void _AdjustCursorSpeed() noexcept;

	void _UpdateCursorClip() noexcept;

	void _StartCapture(POINT cursorPos) noexcept;

	void _StopCapture(POINT cursorPos, bool onDestroy = false) noexcept;

	HCURSOR _hCursor = NULL;
	POINT _cursorPos{};

	RECT _curClips{};
	int _originCursorSpeed = 0;

	bool _isUnderCapture = false;
};

}
