#pragma once

namespace Magpie::Core {

struct ScalingOptions;

class CursorManager {
public:
	CursorManager() noexcept = default;
	CursorManager(const CursorManager&) = delete;
	CursorManager(CursorManager&&) = delete;

	~CursorManager() noexcept;

	bool Initialize(
		HWND hwndSrc,
		HWND hwndScaling,
		const RECT& srcRect,
		const RECT& scalingWndRect,
		const RECT& destRect,
		const ScalingOptions& options
	) noexcept;

	void Update() noexcept;

private:
	void _AdjustCursorSpeed() noexcept;

	void _UpdateCursorClip() noexcept;

	void _StartCapture(POINT cursorPos) noexcept;

	void _StopCapture(POINT cursorPos, bool onDestroy = false) noexcept;

	POINT _SrcToScaling(POINT pt, bool screenCoord) noexcept;
	POINT _ScalingToSrc(POINT pt) noexcept;

	HWND _hwndSrc;
	HWND _hwndScaling;
	RECT _srcRect;
	RECT _scalingWndRect;
	RECT _destRect;

	// 当前帧的光标，光标不可见则为 NULL
	HCURSOR _curCursor = NULL;
	POINT _curCursorPos{ std::numeric_limits<LONG>::max(), std::numeric_limits<LONG>::max() };

	RECT _curClips{};
	int _originCursorSpeed = 0;

	bool _isUnderCapture = false;
	bool _isAdjustCursorSpeed = false;
	bool _isDebugMode = false;
	bool _is3DGameMode = false;
};

}
