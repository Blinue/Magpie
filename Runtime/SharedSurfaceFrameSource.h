#pragma once
#include "FrameSourceBase.h"


class SharedSurfaceFrameSource : public FrameSourceBase {
public:
	SharedSurfaceFrameSource() {}
	virtual ~SharedSurfaceFrameSource() {}

	bool Initialize() override;

	ComPtr<ID3D11Texture2D> GetOutput() override;

	bool Update() override;

private:
	using _DwmGetDxSharedSurfaceFunc = bool(
		HWND hWnd,
		HANDLE* phSurface,
		LUID* pAdapterLuid,
		ULONG* pFmtWindow,
		ULONG* pPresentFlags,
		ULONGLONG* pWin32KUpdateId
	);
	_DwmGetDxSharedSurfaceFunc *_dwmGetDxSharedSurface = nullptr;

	D3D11_BOX _clientInFrame{};
	HWND _hwndSrc = NULL;
	ComPtr<ID3D11DeviceContext4> _d3dDC;
	ComPtr<ID3D11Device5> _d3dDevice;
	ComPtr<ID3D11Texture2D> _output;
};

