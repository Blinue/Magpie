#pragma once
#include "pch.h"
#include "FrameSourceBase.h"


class GDIFrameSource : public FrameSourceBase {
public:
	GDIFrameSource() {};
	virtual ~GDIFrameSource() {}

	bool Initialize() override;

	ComPtr<ID3D11Texture2D> GetOutput() override;

	bool Update() override;

private:
	ComPtr<ID3D11DeviceContext4> _d3dDC;

	HWND _hwndSrc = NULL;
	ComPtr<ID3D11Texture2D> _output;

	RECT _srcClientRect{};
	SIZE _srcClientSize{};
	RECT _srcWndRect{};
	SIZE _srcWndSize{};

	std::vector<BYTE> _pixels;
};
