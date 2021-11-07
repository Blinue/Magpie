#pragma once
#include "pch.h"
#include "FrameSourceBase.h"


class LegacyGDIFrameSource : public FrameSourceBase {
public:
	LegacyGDIFrameSource() {};
	virtual ~LegacyGDIFrameSource() {}

	bool Initialize() override;

	ComPtr<ID3D11Texture2D> GetOutput() override;

	bool Update() override;

	bool HasRoundCornerInWin11() override {
		return false;
	}

private:
	ComPtr<ID3D11DeviceContext> _d3dDC;

	HWND _hwndSrc = NULL;
	ComPtr<ID3D11Texture2D> _output;

	BITMAPINFO _bi{};
	RECT _frameInWindow{};
	std::unique_ptr<BYTE> _pixels;
};
