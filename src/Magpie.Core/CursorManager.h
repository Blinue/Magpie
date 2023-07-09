#pragma once

namespace Magpie::Core {

class CursorManager {
public:
	CursorManager() = default;
	CursorManager(const CursorManager&) = delete;
	CursorManager(CursorManager&&) = delete;

	~CursorManager() noexcept;

	bool Initialize() noexcept;

	std::pair<HCURSOR, POINT> Update() noexcept;

private:
	void _ShowSystemCursor(bool show);

	void _AdjustCursorSpeed() noexcept;

	void _UpdateCursorClip() noexcept;

	void _StartCapture(POINT cursorPos) noexcept;

	void _StopCapture(POINT cursorPos, bool onDestroy = false) noexcept;

	RECT _curClips{};
	int _originCursorSpeed = 0;

	bool _isUnderCapture = false;
};

}
