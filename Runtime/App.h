#pragma once
#include "pch.h"
#include "Renderer.h"
#include "FrameSourceBase.h"


class App {
public:
	~App();

	static App& GetInstance() {
		static App instance;
		return instance;
	}

	bool Initialize(HINSTANCE hInst);

	bool Run(
		HWND hwndSrc,
		const std::string& effectsJson,
		int captureMode,
		bool noCursor,
		bool adjustCursorSpeed,
		bool showFPS,
		bool disableRoundCorner,
		int frameRate,
		bool disableLowLatency,
		bool breakpointMode
	);

	HINSTANCE GetHInstance() const {
		return _hInst;
	}

	HWND GetHwndSrc() const {
		return _hwndSrc;
	}

	const RECT& GetSrcClientRect() const {
		return _srcClientRect;
	}

	HWND GetHwndHost() const {
		return _hwndHost;
	}

	SIZE GetHostWndSize() const {
		return _hostWndSize;
	}

	Renderer& GetRenderer() {
		return *_renderer;
	}

	FrameSourceBase& GetFrameSource() {
		return *_frameSource;
	}

	bool IsNoCursor() const {
		return _noCursor;
	}

	bool IsAdjustCursorSpeed() const {
		return _adjustCursorSpeed;
	}

	bool IsShowFPS() const {
		return _showFPS;
	}

	int GetCaptureMode() const {
		return _captureMode;
	}

	int GetFrameRate() const {
		return _frameRate;
	}

	bool IsDisableLowLatency() const {
		return _disableLowLatency;
	}

	bool IsBreakpointMode() const {
		return _breakpointMode;
	}

	const char* GetErrorMsg() const {
		return _errorMsg;
	}

	void SetErrorMsg(const char* errorMsg) {
		_errorMsg = errorMsg;
	}

	ComPtr<IWICImagingFactory2> GetWICImageFactory();

	bool RegisterTimer(UINT uElapse, std::function<void()> cb);

private:
	App() {}

	void _Run();

	void _RegisterHostWndClass() const;

	// 创建主窗口
	bool _CreateHostWnd();

	static LRESULT CALLBACK _HostWndProcStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	LRESULT _HostWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	void _ReleaseResources();

	const char* _errorMsg = ErrorMessages::GENERIC;

	// 全屏窗口类名
	static constexpr const wchar_t* _HOST_WINDOW_CLASS_NAME = L"Window_Magpie_967EB565-6F73-4E94-AE53-00CC42592A22";
	static const UINT _WM_DESTORYHOST;

	HINSTANCE _hInst = NULL;
	HWND _hwndSrc = NULL;
	HWND _hwndHost = NULL;

	SIZE _hostWndSize{};
	RECT _srcClientRect{};

	int _captureMode = 0;
	bool _noCursor = false;
	bool _adjustCursorSpeed = false;
	bool _showFPS = false;
	int _frameRate = 0;
	bool _disableLowLatency = false;
	bool _breakpointMode = false;

	std::unique_ptr<Renderer> _renderer;
	std::unique_ptr<FrameSourceBase> _frameSource;
	ComPtr<IWICImagingFactory2> _wicImgFactory;

	UINT _nextTimerId = 1;
	// 存储所有计时器回调
	std::vector<std::function<void()>> _timerCbs;
};
