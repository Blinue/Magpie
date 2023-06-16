#pragma once
#include "DeviceResources.h"
#include "EffectDrawer.h"
#include "Win32Utils.h"

namespace Magpie::Core {

struct ScalingOptions;
class DeviceResources;
class FrameSourceBase;

class Renderer {
public:
	Renderer() noexcept;
	~Renderer() noexcept;

	Renderer(const Renderer&) = delete;
	Renderer(Renderer&&) noexcept = default;

	bool Initialize(HWND hwndSrc, HWND hwndScaling, const ScalingOptions& options) noexcept;

	void Render() noexcept;

	const RECT& SrcRect() const noexcept {
		return _srcRect;
	}

	const RECT& DestRect() const noexcept {
		return _destRect;
	}

private:
	bool _CreateSwapChain() noexcept;

	void _BackendThreadProc(const ScalingOptions& options) noexcept;

	bool _InitFrameSource(const ScalingOptions& options) noexcept;

	ID3D11Texture2D* _BuildEffects(const ScalingOptions& options) noexcept;

	HANDLE _CreateSharedTexture(ID3D11Texture2D* effectsOutput) noexcept;

	void _BackendRender(ID3D11Texture2D* effectsOutput) noexcept;

	// 只能由前台线程访问
	DeviceResources _frontendResources;
	winrt::com_ptr<IDXGISwapChain4> _swapChain;
	Win32Utils::ScopedHandle _frameLatencyWaitableObject;
	winrt::com_ptr<ID3D11Texture2D> _backBuffer;
	uint64_t _lastAccessMutexKey = 0;
	POINT _lastCursorPos{ std::numeric_limits<LONG>::max(), std::numeric_limits<LONG>::max() };

	winrt::com_ptr<ID3D11Texture2D> _frontendSharedTexture;
	winrt::com_ptr<IDXGIKeyedMutex> _frontendSharedTextureMutex;
	RECT _destRect{};
	
	std::thread _backendThread;
	
	// 只能由后台线程访问
	DeviceResources _backendResources;
	std::unique_ptr<FrameSourceBase> _frameSource;
	std::vector<EffectDrawer> _effectDrawers;

	winrt::com_ptr<ID3D11Fence> _d3dFence;
	uint64_t _fenceValue = 0;
	Win32Utils::ScopedHandle _fenceEvent;

	winrt::com_ptr<ID3D11Texture2D> _backendSharedTexture;
	winrt::com_ptr<IDXGIKeyedMutex> _backendSharedTextureMutex;

	// 可由所有线程访问
	HWND _hwndSrc = NULL;
	HWND _hwndScaling = NULL;
	SIZE _scalingWndSize{};

	winrt::Windows::System::DispatcherQueueController _backendThreadDqc{ nullptr };

	std::atomic<uint64_t> _sharedTextureMutexKey = 0;

	HANDLE _sharedTextureHandle = NULL;
	RECT _srcRect{};
	// 用于在初始化时同步对 _sharedTextureHandle 和 _srcRect 的访问
	Win32Utils::SRWMutex _mutex;
};

}
