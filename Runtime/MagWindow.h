#pragma once
#include "pch.h"
#include "Utils.h"
#include "D2DContext.h"
#include "RenderManager.h"


// 管理全屏窗口的创建和销毁
class MagWindow {
public:
	// 确保只能同时存在一个全屏窗口
	static std::unique_ptr<MagWindow> $instance;

	static void CreateInstance() {
		$instance.reset(new MagWindow());
	}
	
	// 不可复制，不可移动
	MagWindow(const MagWindow&) = delete;
	MagWindow(MagWindow&&) = delete;

	~MagWindow() {
		UnregisterClass(_HOST_WINDOW_CLASS_NAME, Env::$instance->GetHInstance());
	}


public:
	static std::wstring RunMsgLoop() {
		std::wstring errMsg;

		while (true) {
			MSG msg;
			while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
				if (msg.message == WM_QUIT) {
					return std::move(errMsg);
				}

				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			try {
				if ($instance) {
					$instance->_renderManager->Render();
				}
			} catch (const magpie_exception& e) {
				errMsg = L"渲染出错：" + e.what();
				DestroyWindow(Env::$instance->GetHwndHost());
			} catch (...) {
				errMsg = L"渲染出现未知错误";
				DestroyWindow(Env::$instance->GetHwndHost());
			}
		}
	}

private:
	MagWindow() {
		if ($instance) {
			Debug::Assert(false, L"已存在全屏窗口");
		}

		_RegisterHostWndClass();
		_CreateHostWnd();

		_renderManager.reset(new RenderManager());

		//Debug::ThrowIfWin32Failed(
		ShowWindow(Env::$instance->GetHwndHost(), SW_NORMAL);
		//	L"ShowWindow失败"
		//);

		// 取消全屏窗口的置顶，这样可以使该窗口在最前
		DWORD style = GetWindowStyle(Env::$instance->GetHwndHost());
		style &= ~WS_EX_TOPMOST;
		SetWindowLongW(Env::$instance->GetHwndHost(), GWL_STYLE, style);
	}


	LRESULT _HostWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
		if (message == _WM_DESTORYMAG) {
			DestroyWindow(Env::$instance->GetHwndHost());
			return 0;
		}
		if (message == WM_DESTROY) {
			// 有两个退出路径：
			// 1. 前台窗口发生改变
			// 2. 收到_WM_DESTORYMAG 消息
			$instance = nullptr;
			PostQuitMessage(0);
			return 0;
		} else {
			auto [resolved, rt] = _renderManager->WndProc(hWnd, message, wParam, lParam);

			if (resolved) {
				return rt;
			} else {
				return DefWindowProc(hWnd, message, wParam, lParam);
			}
		}

		return 0;
	}

	static LRESULT CALLBACK _HostWndProcStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
		if (!$instance) {
			return DefWindowProc(hWnd, message, wParam, lParam);
		} else {
			return $instance->_HostWndProc(hWnd, message, wParam, lParam);
		}
	}

	// 注册全屏窗口类
	void _RegisterHostWndClass() const {
		WNDCLASSEX wcex = {};
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.lpfnWndProc = _HostWndProcStatic;
		wcex.hInstance = Env::$instance->GetHInstance();
		wcex.lpszClassName = _HOST_WINDOW_CLASS_NAME;

		// 忽略重复注册造成的错误
		RegisterClassEx(&wcex);
	}

	void _CreateHostWnd() {
		// 创建全屏窗口
		SIZE screenSize = Utils::GetScreenSize(Env::$instance->GetHwndSrc());
		HWND hwndHost = CreateWindowEx(
			WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_LAYERED | WS_EX_TRANSPARENT,
			_HOST_WINDOW_CLASS_NAME, NULL, WS_CLIPCHILDREN | WS_POPUP | WS_VISIBLE,
			0, 0, screenSize.cx, screenSize.cy,
			NULL, NULL, Env::$instance->GetHInstance(), NULL);
		Debug::ThrowIfWin32Failed(hwndHost, L"创建全屏窗口失败");

		// 设置窗口不透明
		Debug::ThrowIfWin32Failed(
			SetLayeredWindowAttributes(hwndHost, 0, 255, LWA_ALPHA),
			L"SetLayeredWindowAttributes 失败"
		);

		Env::$instance->SetHwndHost(hwndHost);
	}


	// 全屏窗口类名
	static constexpr const wchar_t* _HOST_WINDOW_CLASS_NAME = L"Window_Magpie_967EB565-6F73-4E94-AE53-00CC42592A22";

	static UINT _WM_DESTORYMAG;
	
	std::unique_ptr<RenderManager> _renderManager = nullptr;
};
