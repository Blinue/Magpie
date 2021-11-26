#pragma once
#include "FrameSourceBase.h"


// 使用 Magnification API 捕获窗口
// 速度很慢，不支持多显示器
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

