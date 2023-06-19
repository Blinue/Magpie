#pragma once
#include "WinRTUtils.h"

namespace Magpie::Core {

struct ScalingOptions;

class CursorManager {
public:
	CursorManager() = default;
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

	std::pair<HCURSOR, POINT> Update() noexcept;

	winrt::event_token CursorVisibilityChanged(winrt::delegate<bool> const& handler) {
		return _cursorVisibilityChangedEvent.add(handler);
	}

	WinRTUtils::EventRevoker CursorVisibilityChanged(winrt::auto_revoke_t, winrt::delegate<bool> const& handler) {
		winrt::event_token token = CursorVisibilityChanged(handler);
		return WinRTUtils::EventRevoker([this, token]() {
			CursorVisibilityChanged(token);
		});
	}

	void CursorVisibilityChanged(winrt::event_token const& token) {
		_cursorVisibilityChangedEvent.remove(token);
	}

private:
	void _ShowSystemCursor(bool show);

	void _AdjustCursorSpeed() noexcept;

	void _UpdateCursorClip() noexcept;

	void _StartCapture(POINT cursorPos) noexcept;

	void _StopCapture(POINT cursorPos, bool onDestroy = false) noexcept;

	POINT _SrcToScaling(POINT pt, bool screenCoord) noexcept;
	POINT _ScalingToSrc(POINT pt) noexcept;

	HWND _hwndSrc = NULL;
	HWND _hwndScaling = NULL;
	RECT _srcRect{};
	RECT _scalingWndRect{};
	RECT _destRect{};

	winrt::event<winrt::delegate<bool>> _cursorVisibilityChangedEvent;

	RECT _curClips{};
	int _originCursorSpeed = 0;

	bool _isUnderCapture = false;
	bool _isAdjustCursorSpeed = false;
	bool _isDebugMode = false;
	bool _is3DGameMode = false;
	bool _isDrawCursor = false;
};

}
