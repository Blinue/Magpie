#pragma once
#include "pch.h"
#include "FrameSourceBase.h"


class GDIOverDXGIFrameSource : public FrameSourceBase {
public:
	GDIOverDXGIFrameSource() {};
	virtual ~GDIOverDXGIFrameSource() {}

	bool Initialize() override;

	ComPtr<ID3D11Texture2D> GetOutput() override;

	bool Update() override;

	bool HasRoundCornerInWin11() override {
		return false;
	}

private:
	ComPtr<ID3D11DeviceContext> _d3dDC;

	HWND _hwndSrc = NULL;
	ComPtr<IDXGISurface1> _dxgiSurface;
	ComPtr<ID3D11Texture2D> _output;
};
