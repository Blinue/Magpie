#pragma once
#include "pch.h"
#include "FrameSourceBase.h"


class DwmSharedSurfaceFrameSource : public FrameSourceBase {
public:
	DwmSharedSurfaceFrameSource() {}
	virtual ~DwmSharedSurfaceFrameSource() {}

	bool Initialize() override;

	ComPtr<ID3D11Texture2D> GetOutput() override {
		return _output;
	}

	UpdateState Update() override;

	bool HasRoundCornerInWin11() override {
		return false;
	}

	bool IsScreenCapture() override {
		return false;
	}

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

	D3D11_BOX _frameInWnd{};
	ComPtr<ID3D11Texture2D> _output;
};

