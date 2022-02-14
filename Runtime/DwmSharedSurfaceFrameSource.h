#pragma once
#include "pch.h"
#include "FrameSourceBase.h"


class DwmSharedSurfaceFrameSource : public FrameSourceBase {
public:
	DwmSharedSurfaceFrameSource() {}
	virtual ~DwmSharedSurfaceFrameSource() {}

	bool Initialize() override;

	UpdateState Update() override;

	bool IsScreenCapture() override {
		return false;
	}

protected:
	bool _HasRoundCornerInWin11() override {
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
	
};

