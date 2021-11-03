#pragma once
#include "pch.h"
#include "FrameSourceBase.h"


class GDIFrameSource : public FrameSourceBase {
public:
	GDIFrameSource() {};
	virtual ~GDIFrameSource() {}

	bool Initialize(SIZE& frameSize) override;

	ComPtr<ID3D11Texture2D> GetOutput() override;

	bool Update() override;

	bool HasRoundCornerInWin11() override {
		return false;
	}

private:
	ComPtr<ID3D11DeviceContext> _d3dDC;

	HWND _hwndSrc = NULL;
	ComPtr<ID3D11Texture2D> _output;

	RECT _srcClientRect{};
	SIZE _srcClientSize{};
	RECT _srcWndRect{};
	SIZE _srcWndSize{};

	std::vector<BYTE> _pixels;
};
