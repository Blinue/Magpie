#pragma once
#include "ScalingOptions.h"

namespace Magpie {

// 详细信息参见 https://github.com/Blinue/Magpie/pull/1071#issuecomment-2668398139
enum class SrcWindowKind {
	// 系统标题栏和边框，有阴影，左右下三边在窗口外调整大小，Win11 中有圆角
	Native,
	// 无标题栏，系统边框（上边框可能自绘），有阴影，左右下三边在窗口外调整大
	// 小，Win11 中有圆角
	NoTitleBar,
	// 无标题栏，是否有边框取决于 OS 版本（Win10 中不存在边框，Win11 中边框
	// 被绘制到客户区内），有阴影，在窗口内调整大小，Win11 中有圆角
	NoBorder,
	// 无标题栏、边框和阴影，在窗口内调整大小，Win11 中无圆角
	NoDecoration,
	// 无标题栏，系统边框但上边框较粗，有阴影，左右下三边在窗口外调整大小，Win11 中有圆角
	OnlyThickFrame
};

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

	bool IsFocused() const noexcept {
		return _isFocused;
	}

	// IsMaximized 已定义为宏
	bool IsZoomed() const noexcept {
		return _isMaximized;
	}

	SrcWindowKind WindowKind() const noexcept {
		return _windowKind;
	}

private:
	bool _CalcFrameRect(const ScalingOptions& options) noexcept;

	HWND _hWnd = NULL;
	RECT _windowRect{};
	RECT _frameRect{};
	uint32_t _topBorderThicknessInClient = 0;
	SrcWindowKind _windowKind = SrcWindowKind::Native;

	bool _isFocused = false;
	bool _isMaximized = false;
};

}
