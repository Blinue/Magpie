#pragma once
#include "pch.h"
#include <unordered_map>
#include "MagOptions.h"


namespace Magpie::Core {

class DeviceResources;
class Renderer;
class FrameSourceBase;
class CursorManager;

class MagApp {
public:
	~MagApp();

	static MagApp& Get() noexcept {
		static MagApp instance;
		return instance;
	}

	bool Run(
		HWND hwndSrc,
		MagOptions&& options,
		winrt::DispatcherQueue const& dispatcher
	);

	void Stop();

	void ToggleOverlay();

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

	MagOptions& GetOptions() noexcept {
		return _options;
	}

	winrt::DispatcherQueue Dispatcher() const noexcept {
		return _dispatcher;
	}

	// 注册消息回调，回调函数如果不阻断消息应返回空
	UINT RegisterWndProcHandler(std::function<std::optional<LRESULT>(HWND, UINT, WPARAM, LPARAM)> handler);
	void UnregisterWndProcHandler(UINT id);

private:
	MagApp();

	void _RunMessageLoop();

	void _RegisterWndClasses() const;

	// 创建主窗口
	bool _CreateHostWnd();

	bool _InitFrameSource();

	bool _DisableDirectFlip();

	static LRESULT CALLBACK _HostWndProcStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	LRESULT _HostWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	void _OnQuit();

	winrt::DispatcherQueue _dispatcher{ nullptr };

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
	MagOptions _options;

	HHOOK _hKeyboardHook = NULL;

	std::map<UINT, std::function<std::optional<LRESULT>(HWND, UINT, WPARAM, LPARAM)>> _wndProcHandlers;
	UINT _nextWndProcHandlerID = 1;
};

}
