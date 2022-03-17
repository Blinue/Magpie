#pragma once
#include "pch.h"
#include <unordered_map>
#include "ErrorMessages.h"


class DeviceResources;
class Renderer;
class FrameSourceBase;
class CursorManager;


class App {
public:
	~App();

	static App& Get() noexcept {
		static App instance;
		return instance;
	}

	bool Initialize(HINSTANCE hInst);

	bool Run(
		HWND hwndSrc,
		const std::string& effectsJson,
		UINT captureMode,
		float cursorZoomFactor,
		UINT cursorInterpolationMode,
		int adapterIdx,
		UINT multiMonitorMode,
		const RECT& cropBorders,
		UINT flags
	);

	void Quit();

	HINSTANCE GetHInstance() const noexcept {
		return _hInst;
	}

	HWND GetHwndSrc() const noexcept {
		return _hwndSrc;
	}

	HWND GetHwndHost() const noexcept {
		return _hwndHost;
	}

	const RECT& GetHostWndRect() const noexcept {
		return _hostWndRect;
	}

	DeviceResources& GetDeviceResources() noexcept {
		return *_deviceResources;
	}

	Renderer& GetRenderer() noexcept {
		return *_renderer;
	}

	CursorManager& GetCursorManager() noexcept {
		return *_cursorManager;
	}

	FrameSourceBase& GetFrameSource() noexcept {
		return *_frameSource;
	}

	float GetCursorZoomFactor() const noexcept {
		return _cursorZoomFactor;
	}

	UINT GetCursorInterpolationMode() const noexcept {
		return _cursorInterpolationMode;
	}

	int GetAdapterIdx() const noexcept {
		return _adapterIdx;
	}

	UINT GetMultiMonitorUsage() const noexcept {
		return _multiMonitorUsage;
	}

	const RECT& GetCropBorders() const noexcept {
		return _cropBorders;
	}

	bool IsMultiMonitorMode() const noexcept {
		return _isMultiMonitorMode;
	}

	bool IsNoCursor() const noexcept {
		return _flags & (UINT)_FlagMasks::NoCursor;
	}

	bool IsAdjustCursorSpeed() const noexcept {
		return _flags & (UINT)_FlagMasks::AdjustCursorSpeed;
	}

	bool IsDisableLowLatency() const noexcept {
		return _flags & (UINT)_FlagMasks::DisableLowLatency;
	}

	bool IsDisableWindowResizing() const noexcept {
		return _flags & (UINT)_FlagMasks::DisableWindowResizing;
	}

	bool IsBreakpointMode() const noexcept {
		return _flags & (UINT)_FlagMasks::BreakpointMode;
	}

	bool IsDisableDirectFlip() const noexcept {
		return _flags & (UINT)_FlagMasks::DisableDirectFlip;
	}

	bool IsConfineCursorIn3DGames() const noexcept {
		return _flags & (UINT)_FlagMasks::ConfineCursorIn3DGames;
	}

	bool IsCropTitleBarOfUWP() const noexcept {
		return _flags & (UINT)_FlagMasks::CropTitleBarOfUWP;
	}

	bool IsDisableEffectCache() const noexcept {
		return _flags & (UINT)_FlagMasks::DisableEffectCache;
	}

	bool IsSimulateExclusiveFullscreen() const noexcept {
		return _flags & (UINT)_FlagMasks::SimulateExclusiveFullscreen;
	}

	bool IsDisableVSync() const noexcept {
		return _flags & (UINT)_FlagMasks::DisableVSync;
	}

	bool IsSaveEffectSources() const noexcept {
		return _flags & (UINT)_FlagMasks::SaveEffectSources;
	}

	bool IsTreatWarningsAsErrors() const noexcept {
		return _flags & (UINT)_FlagMasks::WarningsAreErrors;
	}

	const char* GetErrorMsg() const noexcept {
		return _errorMsg;
	}

	void SetErrorMsg(const char* errorMsg) noexcept {
		_errorMsg = errorMsg;
	}

	winrt::com_ptr<IWICImagingFactory2> GetWICImageFactory();

	UINT RegisterWndProcHandler(std::function<bool(HWND, UINT, WPARAM, LPARAM)> handler);
	void UnregisterWndProcHandler(UINT id);

private:
	App();

	void _RunMessageLoop();

	void _RegisterWndClasses() const;

	// 创建主窗口
	bool _CreateHostWnd();

	bool _InitFrameSource(int captureMode);

	bool _DisableDirectFlip();

	static LRESULT CALLBACK _HostWndProcStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	LRESULT _HostWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	void _OnQuit();

	const char* _errorMsg = ErrorMessages::GENERIC;

	HINSTANCE _hInst = NULL;
	HWND _hwndSrc = NULL;
	HWND _hwndHost = NULL;

	// 关闭 DirectFlip 时的背景全屏窗口
	HWND _hwndDDF = NULL;

	RECT _hostWndRect{};

	float _cursorZoomFactor = 0;
	UINT _cursorInterpolationMode = 0;
	int _adapterIdx = 0;
	UINT _multiMonitorUsage = 0;
	UINT _flags = 0;
	RECT _cropBorders{};

	enum class _FlagMasks : UINT {
		NoCursor = 0x1,
		AdjustCursorSpeed = 0x2,
		SaveEffectSources = 0x4,
		SimulateExclusiveFullscreen = 0x8,
		DisableLowLatency = 0x10,
		BreakpointMode = 0x20,
		DisableWindowResizing = 0x40,
		DisableDirectFlip = 0x80,
		ConfineCursorIn3DGames = 0x100,
		CropTitleBarOfUWP = 0x200,
		DisableEffectCache = 0x400,
		DisableVSync = 0x800,
		WarningsAreErrors = 0x1000
	};

	// 多屏幕模式下光标可以在屏幕间自由移动
	bool _isMultiMonitorMode = false;

	bool _windowResizingDisabled = false;
	bool _roundCornerDisabled = false;

	std::unique_ptr<DeviceResources> _deviceResources;
	std::unique_ptr<Renderer> _renderer;
	std::unique_ptr<FrameSourceBase> _frameSource;
	std::unique_ptr<CursorManager> _cursorManager;

	std::map<UINT, std::function<bool(HWND, UINT, WPARAM, LPARAM)>> _wndProcHandlers;
	UINT _nextWndProcHandlerID = 1;
};
