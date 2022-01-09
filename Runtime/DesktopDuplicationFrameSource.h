#pragma once
#include "FrameSourceBase.h"


class DesktopDuplicationFrameSource : public FrameSourceBase {
public:
	DesktopDuplicationFrameSource() {};
	virtual ~DesktopDuplicationFrameSource();

	bool Initialize() override;

	ComPtr<ID3D11Texture2D> GetOutput() override {
		return _output;
	}

	UpdateState Update() override;

	bool HasRoundCornerInWin11() override {
		return true;
	}

	bool IsScreenCapture() override {
		return true;
	}

private:
	static DWORD WINAPI _DDPThreadProc(LPVOID lpThreadParameter);

	// 消除刚进入全屏时短暂的黑屏
	bool _firstFrame = true;
	ComPtr<ID3D11Texture2D> _output;
	ComPtr<IDXGIOutputDuplication> _outputDup;
	std::vector<BYTE> _dupMetaData;

	RECT _srcClientInMonitor{};
	D3D11_BOX _frameInMonitor{};
};

