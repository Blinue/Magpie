#pragma once
#include "FrameSourceBase.h"


// 使用 Desktop Duplication API 捕获窗口
// 在单独的线程中接收屏幕帧以避免丢帧
class DesktopDuplicationFrameSource : public FrameSourceBase {
public:
	DesktopDuplicationFrameSource() {};
	virtual ~DesktopDuplicationFrameSource();

	bool Initialize() override;

	UpdateState Update() override;

	bool IsScreenCapture() override {
		return true;
	}

protected:
	bool _HasRoundCornerInWin11() override {
		return true;
	}

private:
	bool _InitializeDdpD3D(HANDLE hSharedTex);

	static DWORD WINAPI _DDPThreadProc(LPVOID lpThreadParameter);

	winrt::com_ptr<IDXGIOutputDuplication> _outputDup;
	std::vector<BYTE> _dupMetaData;

	HANDLE _hDDPThread = NULL;
	std::atomic<bool> _exiting = false;
	// 0: 等待新帧
	// 1: 新帧到达
	// 2: 等待第一帧
	std::atomic<UINT> _newFrameState = 2;

	// DDP 线程使用的 D3D 设备
	winrt::com_ptr<ID3D11Device> _ddpD3dDevice;
	winrt::com_ptr<ID3D11DeviceContext> _ddpD3dDC;

	// 这些均指向同一个纹理
	// 用于在 D3D Device 间同步对该纹理的访问
	winrt::com_ptr<ID3D11Texture2D> _sharedTex;
	winrt::com_ptr<IDXGIKeyedMutex> _sharedTexMutex;
	winrt::com_ptr<ID3D11Texture2D> _ddpSharedTex;
	winrt::com_ptr<IDXGIKeyedMutex> _ddpSharedTexMutex;

	RECT _srcClientInMonitor{};
	D3D11_BOX _frameInMonitor{};
};

