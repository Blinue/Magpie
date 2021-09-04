#pragma once
#include "pch.h"


class FrameSourceBase {
public:
	FrameSourceBase() {}

	virtual ~FrameSourceBase() {}

	// 不可复制，不可移动
	FrameSourceBase(const FrameSourceBase&) = delete;
	FrameSourceBase(FrameSourceBase&&) = delete;

	virtual bool Initialize() = 0;

	virtual ComPtr<ID3D11Texture2D> GetOutput() = 0;

	virtual bool Update() = 0;
};
