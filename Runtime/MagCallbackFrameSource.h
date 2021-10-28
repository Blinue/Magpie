#pragma once
#include "FrameSourceBase.h"


class MagCallbackFrameSource : public FrameSourceBase {
public:
	MagCallbackFrameSource() {};
	virtual ~MagCallbackFrameSource();

	bool Initialize() override;

	ComPtr<ID3D11Texture2D> GetOutput() override;

	bool Update() override;

	bool HasRoundCornerInWin11() override {
		return true;
	}

private:
	static BOOL CALLBACK _ImageScalingCallback(
		HWND hWnd,
		void* srcdata,
		MAGIMAGEHEADER srcheader,
		void* destdata,
		MAGIMAGEHEADER destheader,
		RECT unclipped,
		RECT clipped,
		HRGN dirty
	);

	HWND _hwndMag = NULL;
	ComPtr<ID3D11Texture2D> _output;
};

