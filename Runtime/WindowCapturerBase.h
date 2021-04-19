#pragma once
#include "pch.h"
#include <variant>

class WindowCapturerBase {
public:
	WindowCapturerBase() {}

	virtual ~WindowCapturerBase() {}

	WindowCapturerBase(const WindowCapturerBase&) = delete;
	WindowCapturerBase(WindowCapturerBase&&) = delete;

	virtual std::variant<ComPtr<IWICBitmapSource>, ComPtr<ID2D1Bitmap1>> GetFrame() = 0;
};
