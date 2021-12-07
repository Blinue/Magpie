#pragma once
#include "FrameSourceBase.h"


class GDIScreenFrameSource : public FrameSourceBase {
public:
	GDIScreenFrameSource() {};
	virtual ~GDIScreenFrameSource() {}

	bool Initialize() override;

	ComPtr<ID3D11Texture2D> GetOutput() override {
		return _output;
	}

	bool Update() override;

	bool HasRoundCornerInWin11() override {
		return true;
	}

private:
	ComPtr<ID3D11DeviceContext> _d3dDC;
	HDC _hdcScreen;

	SIZE _frameSize{};
	ComPtr<IDXGISurface1> _dxgiSurface;
	ComPtr<ID3D11Texture2D> _output;
};

