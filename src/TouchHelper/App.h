#pragma once

class App {
public:
	static App& Get() noexcept {
		static App instance;
		return instance;
	}

	bool Initialzie() noexcept;

	int Run() noexcept;

private:
	bool  _CheckSingleInstance() noexcept;

	bool _CheckMagpieRunning() noexcept;

	bool _CreateMsgWindow() noexcept;

	static LRESULT CALLBACK _WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	LRESULT _MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

	bool _UpdateInputTransform(bool onTimer = false) noexcept;

	void _DisableInputTransform() const noexcept;

	void _Uninitialize() noexcept;

	wil::unique_mutex_nothrow _hSingleInstanceMutex;
	wil::unique_mutex_nothrow _hMagpieMutex;
	wil::unique_hwnd _hwndMsg;

	HWND _hwndScaling = NULL;

	bool _isMagInitialized = false;
};
