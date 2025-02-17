#pragma once
#include "DeviceResources.h"
#include "BackendDescriptorStore.h"
#include "EffectDrawer.h"
#include "Win32Helper.h"
#include "CursorDrawer.h"
#include "StepTimer.h"
#include "EffectsProfiler.h"
#include "ScalingError.h"

namespace Magpie {

class FrameSourceBase;

class Renderer {
public:
	Renderer() noexcept;
	~Renderer() noexcept;

	Renderer(const Renderer&) = delete;
	Renderer(Renderer&&) = delete;

	ScalingError Initialize(HWND hwndSwapChain) noexcept;

	bool Render() noexcept;

	bool IsOverlayVisible() noexcept;

	void SetOverlayVisibility(bool value, bool noSetForeground = false) noexcept;

	const RECT& SrcRect() const noexcept;

	// 屏幕坐标而不是窗口局部坐标
	const RECT& DestRect() const noexcept {
		return _destRect;
	}

	const FrameSourceBase& FrameSource() const noexcept {
		return *_frameSource;
	}

	void OnCursorVisibilityChanged(bool isVisible, bool onDestory);

	void MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

	struct EffectInfo {
		std::string name;
		std::vector<std::string> passNames;
		bool isFP16 = false;
	};
	const std::vector<EffectInfo>& EffectInfos() const noexcept {
		return _effectInfos;
	}

private:
	bool _CreateSwapChain(HWND hwndSwapChain) noexcept;

	void _FrontendRender() noexcept;

	void _BackendThreadProc() noexcept;

	ID3D11Texture2D* _InitBackend() noexcept;

	bool _InitFrameSource() noexcept;

	ID3D11Texture2D* _BuildEffects() noexcept;

	HANDLE _CreateSharedTexture(ID3D11Texture2D* effectsOutput) noexcept;

	void _BackendRender(ID3D11Texture2D* effectsOutput) noexcept;

	bool _UpdateDynamicConstants() const noexcept;

	static LRESULT CALLBACK _LowLevelKeyboardHook(int nCode, WPARAM wParam, LPARAM lParam);

	// 只能由前台线程访问
	DeviceResources _frontendResources;
	winrt::com_ptr<IDXGISwapChain4> _swapChain;
	wil::unique_event_nothrow _frameLatencyWaitableObject;
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

	wil::unique_hhook _hKeyboardHook;
	
	// 只能由后台线程访问
	DeviceResources _backendResources;
	Magpie::BackendDescriptorStore _backendDescriptorStore;
	std::unique_ptr<FrameSourceBase> _frameSource;
	std::vector<EffectDrawer> _effectDrawers;

	StepTimer _stepTimer;
	EffectsProfiler _effectsProfiler;

	winrt::com_ptr<ID3D11Fence> _d3dFence;
	uint64_t _fenceValue = 0;
	wil::unique_event_nothrow _fenceEvent;

	winrt::com_ptr<ID3D11Texture2D> _backendSharedTexture;
	winrt::com_ptr<IDXGIKeyedMutex> _backendSharedTextureMutex;

	winrt::com_ptr<ID3D11Buffer> _dynamicCB;

	// 可由所有线程访问
	std::atomic<uint64_t> _sharedTextureMutexKey = 0;

	// INVALID_HANDLE_VALUE 表示后端初始化失败
	std::atomic<HANDLE> _sharedTextureHandle{ NULL };
	// 下面三个成员由 _sharedTextureHandle 同步
	winrt::Windows::System::DispatcherQueue _backendThreadDispatcher{ nullptr };
	ScalingError _backendInitError = ScalingError::NoError;

	// 供游戏内叠加层使用
	// 由于要跨线程访问，初始化之后不能更改
	std::vector<EffectInfo> _effectInfos;
};

}
