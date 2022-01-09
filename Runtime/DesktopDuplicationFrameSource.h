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

	ComPtr<ID3D11Texture2D> _output;
	ComPtr<IDXGIOutputDuplication> _outputDup;
	std::vector<BYTE> _dupMetaData;

	HANDLE _hDDPThread = NULL;
	HANDLE _hTerminateThreadEvent = NULL;
	ComPtr<ID3D11Device> _ddpD3dDevice;
	ComPtr<ID3D11DeviceContext> _ddpD3dDC;

	ComPtr<ID3D11Texture2D> _sharedTex;
	ComPtr<IDXGIKeyedMutex> _sharedTexMutex;
	ComPtr<ID3D11Texture2D> _ddpSharedTex;
	ComPtr<IDXGIKeyedMutex> _ddpSharedTexMutex;

	RECT _srcClientInMonitor{};
	D3D11_BOX _frameInMonitor{};
};

