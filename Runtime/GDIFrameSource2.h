#pragma once
#include "FrameSourceBase.h"


class GDIFrameSource2 : public FrameSourceBase {
public:
	GDIFrameSource2() {};
	virtual ~GDIFrameSource2() {}

	bool Initialize() override;

	ComPtr<ID3D11Texture2D> GetOutput() override;

	bool Update() override;

private:
	ComPtr<ID3D11DeviceContext4> _d3dDC;

	HWND _hwndSrc = NULL;
	ComPtr<ID3D11Texture2D> _output;
};
