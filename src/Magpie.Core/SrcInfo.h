#pragma once
#include "ScalingOptions.h"

namespace Magpie {

class SrcInfo {
public:
	SrcInfo() = default;

	// 防止意外复制
	SrcInfo(const SrcInfo&) = delete;
	SrcInfo(SrcInfo&&) = delete;

	bool Set(HWND hWnd, const ScalingOptions& options) noexcept;

	bool UpdateState(HWND hwndFore) noexcept;

	HWND Handle() const noexcept {
		return _hWnd;
	}

	const RECT& WindowRect() const noexcept {
		return _windowRect;
	}

	const RECT& FrameRect() const noexcept {
		return _frameRect;
	}

	uint32_t BorderThickness() const noexcept {
		return _borderThickness;
	}

	bool IsFocused() const noexcept {
		return _isFocused;
	}

	// IsMaximized 已定义为宏
	bool IsZoomed() const noexcept {
		return _isMaximized;
	}

private:
	bool _CalcFrameRect(const ScalingOptions& options) noexcept;

	HWND _hWnd = NULL;
	RECT _windowRect{};
	RECT _frameRect{};
	uint32_t _borderThickness = 0;

	bool _isFocused = false;
	bool _isMaximized = false;
};

}
