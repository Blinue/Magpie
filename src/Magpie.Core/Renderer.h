#pragma once
#include "DeviceResources.h"
#include "BackendDescriptorStore.h"
#include "EffectDrawer.h"
#include "Win32Helper.h"
#include "CursorDrawer.h"
#include "StepTimer.h"
#include "EffectsProfiler.h"
#include "ScalingError.h"
#include "PresenterBase.h"
#include "OverlayDrawer.h"

namespace Magpie {

class FrameSourceBase;

class Renderer {
public:
	Renderer() noexcept;
	~Renderer() noexcept;

	Renderer(const Renderer&) = delete;
	Renderer(Renderer&&) = delete;

	ScalingError Initialize(HWND hwndAttach) noexcept;

	bool Render() noexcept;

	bool Resize() noexcept;

	void MoveSwapChain() noexcept;

	bool IsOverlayVisible() noexcept;

	void IsOverlayVisible(bool value) noexcept;

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

	const std::vector<const EffectDesc*>& ActiveEffectDescs() const noexcept {
		return _activeEffectDescs;
	}

	void StartProfile() noexcept;

	void StopProfile() noexcept;

	bool IsCursorOnOverlayCaptionArea() const noexcept {
		return _overlayDrawer.IsCursorOnCaptionArea();
	}

private:
	bool _InitPresenter(HWND hwndAttach) noexcept;

	void _FrontendRender() noexcept;

	void _BackendThreadProc() noexcept;

	ID3D11Texture2D* _InitBackend() noexcept;

	bool _InitFrameSource() noexcept;

	ID3D11Texture2D* _BuildEffects() noexcept;

	void _UpdateActiveEffectDescs() noexcept;

	bool _ShouldAppendBicubic(ID3D11Texture2D* outTexture) noexcept;

	bool _AppendBicubic(ID3D11Texture2D** inOutTexture) noexcept;

	ID3D11Texture2D* _ResizeEffects() noexcept;

	void _UpdateDestRect() noexcept;

	HANDLE _CreateSharedTexture(ID3D11Texture2D* effectsOutput) noexcept;

	void _BackendRender(ID3D11Texture2D* effectsOutput) noexcept;

	bool _UpdateDynamicConstants() const noexcept;

	static LRESULT CALLBACK _LowLevelKeyboardHook(int nCode, WPARAM wParam, LPARAM lParam);

	// 只能由前台线程访问
	DeviceResources _frontendResources;
	std::unique_ptr<PresenterBase> _presenter;
	
	CursorDrawer _cursorDrawer;
	OverlayDrawer _overlayDrawer;

	winrt::com_ptr<ID3D11Texture2D> _frontendSharedTexture;
	winrt::com_ptr<IDXGIKeyedMutex> _frontendSharedTextureMutex;
	uint64_t _lastAccessMutexKey = 0;
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
	// 下面四个成员由 _sharedTextureHandle 同步
	winrt::Windows::System::DispatcherQueue _backendThreadDispatcher{ nullptr };
	ScalingError _backendInitError = ScalingError::NoError;
	std::vector<EffectDesc> _effectDescs;
	// 包含追加的 Bicubic
	std::vector<const EffectDesc*> _activeEffectDescs;
};

}
