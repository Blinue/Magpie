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

private:
	ComPtr<ID3D11DeviceContext4> _d3dDC;

	HWND _hwndSrc = NULL;
	ComPtr<IDXGISurface1> _dxgiSurface;
	ComPtr<ID3D11Texture2D> _output;
};
