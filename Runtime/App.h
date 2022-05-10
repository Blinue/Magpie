#pragma once
#include "pch.h"
#include <unordered_map>
#include "ErrorMessages.h"


class DeviceResources;
class Renderer;
class FrameSourceBase;
class CursorManager;
class Config;


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
		UINT multiMonitorUsage,
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

	FrameSourceBase& GetFrameSource() noexcept {
		return *_frameSource;
	}

	CursorManager& GetCursorManager() noexcept {
		return *_cursorManager;
	}

	Config& GetConfig() noexcept {
		return *_config;
	}

	const char* GetErrorMsg() const noexcept {
		return _errorMsg;
	}

	void SetErrorMsg(const char* errorMsg) noexcept {
		_errorMsg = errorMsg;
	}

	winrt::com_ptr<IWICImagingFactory2> GetWICImageFactory();

	// 注册消息回调，回调函数如果不阻断消息应返回空
	UINT RegisterWndProcHandler(std::function<std::optional<LRESULT>(HWND, UINT, WPARAM, LPARAM)> handler);
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

	bool _windowResizingDisabled = false;
	bool _roundCornerDisabled = false;

	std::unique_ptr<DeviceResources> _deviceResources;
	std::unique_ptr<Renderer> _renderer;
	std::unique_ptr<FrameSourceBase> _frameSource;
	std::unique_ptr<CursorManager> _cursorManager;
	std::unique_ptr<Config> _config;

	std::map<UINT, std::function<std::optional<LRESULT>(HWND, UINT, WPARAM, LPARAM)>> _wndProcHandlers;
	UINT _nextWndProcHandlerID = 1;
};
