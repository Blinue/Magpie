#pragma once
#include "pch.h"
#include "D2DContext.h"

class WindowCapturerBase {
public:
	WindowCapturerBase(D2DContext& d2dContext): _d2dContext(d2dContext) {}

	virtual ~WindowCapturerBase() {}

	WindowCapturerBase(const WindowCapturerBase&) = delete;
	WindowCapturerBase(WindowCapturerBase&&) = delete;

	virtual ComPtr<ID2D1Bitmap> GetFrame() = 0;

protected:
	D2DContext& _d2dContext;
};
