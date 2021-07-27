#pragma once
#include "pch.h"
#include "D2DContext.h"


enum class CaptureredFrameType {
	D2DImage, WICBitmap
};

enum class CaptureStyle {
	Normal, Event
};


// 所有类型的 WindowCapturer 的基类
class WindowCapturerBase {
public:
	WindowCapturerBase() {}

	virtual ~WindowCapturerBase() {}

	// 不可复制，不可移动
	WindowCapturerBase(const WindowCapturerBase&) = delete;
	WindowCapturerBase(WindowCapturerBase&&) = delete;

	// 捕获一帧
	virtual ComPtr<IUnknown> GetFrame() = 0;

	// 捕获的帧的类型
	virtual CaptureredFrameType GetFrameType() = 0;
};
