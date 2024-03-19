#pragma once
#include "DeviceResources.h"
#include "BackendDescriptorStore.h"
#include "EffectDrawer.h"
#include "Win32Utils.h"
#include "CursorDrawer.h"
#include "StepTimer.h"
#include "EffectsProfiler.h"

namespace Magpie::Core {

class FrameSourceBase;

class Renderer {
public:
	Renderer() noexcept;
	~Renderer() noexcept;

	Renderer(const Renderer&) = delete;
	Renderer(Renderer&&) = delete;

	bool Initialize() noexcept;

	void Render() noexcept;

	bool IsOverlayVisible() noexcept;

	void SetOverlayVisibility(bool value, bool noSetForeground = false) noexcept;

	const RECT& SrcRect() const noexcept {
		return _srcRect;
	}

	// 屏幕坐标而不是窗口局部坐标
	const RECT& DestRect() const noexcept {
		return _destRect;
	}

	const FrameSourceBase& FrameSource() const noexcept {
		return *_frameSource;
	}

	void OnCursorVisibilityChanged(bool isVisible);

	void MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

	struct EffectInfo {
		std::string name;
		std::vector<std::string> passNames;
	};
	const std::vector<EffectInfo>& EffectInfos() const noexcept {
		return _effectInfos;
	}

private:
	bool _CreateSwapChain() noexcept;

	void _FrontendRender() noexcept;

	void _BackendThreadProc() noexcept;

	ID3D11Texture2D* _InitBackend() noexcept;

	bool _InitFrameSource() noexcept;

	ID3D11Texture2D* _BuildEffects() noexcept;

	HANDLE _CreateSharedTexture(ID3D11Texture2D* effectsOutput) noexcept;

	void _BackendRender(ID3D11Texture2D* effectsOutput, bool noChange) noexcept;

	bool _UpdateDynamicConstants() const noexcept;

	// 只能由前台线程访问
	DeviceResources _frontendResources;
	winrt::com_ptr<IDXGISwapChain4> _swapChain;
	Win32Utils::ScopedHandle _frameLatencyWaitableObject;
	winrt::com_ptr<ID3D11Texture2D> _backBuffer;
	winrt::com_ptr<ID3D11RenderTargetView> _backBufferRtv;
	uint64_t _lastAccessMutexKey = 0;

	CursorDrawer _cursorDrawer;
	std::unique_ptr<class OverlayDrawer> _overlayDrawer;

	HCURSOR _lastCursorHandle = NULL;
	POINT _lastCursorPos{ std::numeric_limits<LONG>::max(), std::numeric_limits<LONG>::max() };
	uint32_t _lastFPS = std::numeric_limits<uint32_t>::max();

	winrt::com_ptr<ID3D11Texture2D> _frontendSharedTexture;
	winrt::com_ptr<IDXGIKeyedMutex> _frontendSharedTextureMutex;
	RECT _destRect{};
	
	std::thread _backendThread;
	
	// 只能由后台线程访问
	DeviceResources _backendResources;
	Magpie::Core::BackendDescriptorStore _backendDescriptorStore;
	std::unique_ptr<FrameSourceBase> _frameSource;
	std::vector<EffectDrawer> _effectDrawers;

	StepTimer _stepTimer;
	EffectsProfiler _effectsProfiler;

	winrt::com_ptr<ID3D11Fence> _d3dFence;
	uint64_t _fenceValue = 0;
	Win32Utils::ScopedHandle _fenceEvent;

	winrt::com_ptr<ID3D11Texture2D> _backendSharedTexture;
	winrt::com_ptr<IDXGIKeyedMutex> _backendSharedTextureMutex;

	winrt::com_ptr<ID3D11Buffer> _dynamicCB;
	uint32_t _firstDynamicEffectIdx = std::numeric_limits<uint32_t>::max();

	// 可由所有线程访问
	winrt::Windows::System::DispatcherQueue _backendThreadDispatcher{ nullptr };

	std::atomic<uint64_t> _sharedTextureMutexKey = 0;

	// INVALID_HANDLE_VALUE 表示后端初始化失败
	HANDLE _sharedTextureHandle = NULL;
	RECT _srcRect{};
	// 用于在初始化时同步对 _sharedTextureHandle 和 _srcRect 的访问
	Win32Utils::SRWMutex _mutex;

	// 供游戏内叠加层使用
	// 由于要跨线程访问，初始化之后不能更改
	std::vector<EffectInfo> _effectInfos;
};

}
