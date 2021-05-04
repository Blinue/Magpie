#pragma once
#include "pch.h"
#include "D2DContext.h"


enum class CaptureredFrameType {
	D2DImage, WICBitmap
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

	// 有的实现无法获知什么时候会有新的帧，因此它们会在新帧到达的时候自动发送渲染消息
	virtual bool IsAutoRender() = 0;

	// 捕获的帧的类型
	virtual CaptureredFrameType GetFrameType() = 0;
};
