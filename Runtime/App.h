#pragma once
#include "pch.h"
#include "Renderer.h"


class App {
public:
	static App* GetInstance() {
		static App* instance = new App();
		return instance;
	}

	bool Initialize(
		std::shared_ptr<spdlog::logger> logger,
		HINSTANCE hInst,
		HWND hwndSrc
	);

	void Run();

	std::shared_ptr<spdlog::logger> GetLogger() const {
		return _logger;
	}

	HINSTANCE GetHInstance() const {
		return _hInst;
	}

	HWND GetHwndSrc() const {
		return _hwndSrc;
	}

	HWND GetHwndHost() const {
		return _hwndHost;
	}

	SIZE GetHostWndSize() const {
		return _hostWndSize;
	}

private:
	App() {}

	void _RegisterHostWndClass() const;

	// 创建主窗口
	bool _CreateHostWnd();

	static LRESULT CALLBACK _HostWndProcStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	LRESULT _HostWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	// 全屏窗口类名
	static constexpr const wchar_t* _HOST_WINDOW_CLASS_NAME = L"Window_Magpie_967EB565-6F73-4E94-AE53-00CC42592A22";
	static const UINT _WM_DESTORYHOST;

	HINSTANCE _hInst = NULL;
	HWND _hwndSrc = NULL;
	HWND _hwndHost = NULL;

	SIZE _hostWndSize{};

	std::unique_ptr<Renderer> _renderer;

	std::shared_ptr<spdlog::logger> _logger = nullptr;
};
