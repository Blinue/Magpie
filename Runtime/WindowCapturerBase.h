#pragma once
#include "pch.h"
#include "D2DContext.h"


enum class CaptureredFrameType {
	D2DImage, WICBitmap
};


class WindowCapturerBase {
public:
	WindowCapturerBase(D2DContext& d2dContext): _d2dContext(d2dContext) {}

	virtual ~WindowCapturerBase() {}

	WindowCapturerBase(const WindowCapturerBase&) = delete;
	WindowCapturerBase(WindowCapturerBase&&) = delete;

	virtual ComPtr<IUnknown> GetFrame() = 0;

	virtual bool IsAutoRender() = 0;

	virtual CaptureredFrameType GetFrameType() = 0;

protected:
	D2DContext& _d2dContext;
};
