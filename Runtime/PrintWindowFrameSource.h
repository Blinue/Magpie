#pragma once
#include "pch.h"
#include "FrameSourceBase.h"


class PrintWindowFrameSource : public FrameSourceBase {
public:
	PrintWindowFrameSource() {}

	virtual ~PrintWindowFrameSource() {}

	bool Initialize() override;

	ComPtr<ID3D11Texture2D> GetOutput() override {
		return _output;
	}

	bool Update() override;

	bool HasRoundCornerInWin11() override {
		return true;
	}

protected:
	ComPtr<ID3D11DeviceContext> _d3dDC;

	ComPtr<IDXGISurface1> _outputSurface;
	ComPtr<ID3D11Texture2D> _output;

	D3D11_BOX _clientRect{};
	ComPtr<ID3D11Texture2D> _windowFrame;
	ComPtr<IDXGISurface1> _windowFrameSurface;
};

