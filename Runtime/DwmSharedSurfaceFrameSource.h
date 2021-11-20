#pragma once
#include "pch.h"
#include "FrameSourceBase.h"


class DwmSharedSurfaceFrameSource : public FrameSourceBase {
public:
	DwmSharedSurfaceFrameSource() {}
	virtual ~DwmSharedSurfaceFrameSource() {}

	bool Initialize() override;

	ComPtr<ID3D11Texture2D> GetOutput() override;

	bool Update() override;

	bool HasRoundCornerInWin11() override {
		return false;
	}

private:
	bool _CalcFrameSize(SIZE& frameSize);

	using _DwmGetDxSharedSurfaceFunc = bool(
		HWND hWnd,
		HANDLE* phSurface,
		LUID* pAdapterLuid,
		ULONG* pFmtWindow,
		ULONG* pPresentFlags,
		ULONGLONG* pWin32KUpdateId
	);
	_DwmGetDxSharedSurfaceFunc *_dwmGetDxSharedSurface = nullptr;

	D3D11_BOX _frameInWnd{};
	HWND _hwndSrc = NULL;
	ComPtr<ID3D11DeviceContext> _d3dDC;
	ComPtr<ID3D11Device> _d3dDevice;
	ComPtr<ID3D11Texture2D> _output;
};

