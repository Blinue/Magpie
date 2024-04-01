#pragma once
#include "FrameSourceBase.h"

namespace Magpie::Core {

class DesktopDuplicationFrameSource final : public FrameSourceBase {
public:
	virtual ~DesktopDuplicationFrameSource();

	bool IsScreenCapture() const noexcept override {
		return true;
	}

	bool CanWaitForFrame() const noexcept override {
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

	DWORD _backendThreadId = 0;
	HANDLE _hDDPThread = NULL;
	std::atomic<bool> _exiting = false;

	uint64_t _lastAccessMutexKey = 0;
	std::atomic<uint64_t> _sharedTextureMutexKey = 0;

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
