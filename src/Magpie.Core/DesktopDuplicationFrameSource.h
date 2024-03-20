#pragma once
#include "FrameSourceBase.h"

namespace Magpie::Core {

class DesktopDuplicationFrameSource final : public FrameSourceBase {
public:
	virtual ~DesktopDuplicationFrameSource();

	bool IsScreenCapture() const noexcept override {
		return true;
	}

	const char* Name() const noexcept override {
		return "Desktop Duplication";
	}

protected:
	bool _HasRoundCornerInWin11() noexcept override {
		return true;
	}

	bool _CanCaptureTitleBar() noexcept override {
		return true;
	}

	bool _Initialize() noexcept override;

	UpdateState _Update() noexcept override;

private:
	bool _InitializeDdpD3D(HANDLE hSharedTex);

	static DWORD WINAPI _DDPThreadProc(LPVOID lpThreadParameter);

	winrt::com_ptr<IDXGIOutputDuplication> _outputDup;

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

}
