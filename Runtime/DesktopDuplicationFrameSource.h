#pragma once
#include "FrameSourceBase.h"


class DesktopDuplicationFrameSource : public FrameSourceBase {
public:
	DesktopDuplicationFrameSource() {};
	virtual ~DesktopDuplicationFrameSource() {}

	bool Initialize() override;

	ComPtr<ID3D11Texture2D> GetOutput() override {
		return _output;
	}

	bool Update() override;

	bool HasRoundCornerInWin11() override {
		return true;
	}

private:
	ComPtr<IDXGIResource> _dxgiRes;
	ComPtr<ID3D11Texture2D> _output;
	ComPtr<IDXGIOutputDuplication> _outputDup;

	D3D11_BOX _frameInMonitor{};
};

