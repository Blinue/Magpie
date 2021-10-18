#pragma once
#include "pch.h"


extern std::shared_ptr<spdlog::logger> logger;


struct Utils {
	static UINT GetWindowShowCmd(HWND hwnd) {
		assert(hwnd != NULL);

		WINDOWPLACEMENT wp{};
		wp.length = sizeof(wp);
		if (!GetWindowPlacement(hwnd, &wp)) {
			SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetWindowPlacement 出错"));
			assert(false);
		}

		return wp.showCmd;
	}

	static RECT GetClientScreenRect(HWND hWnd) {
		RECT r;
		if (!GetClientRect(hWnd, &r)) {
			SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetClientRect 出错"));
			assert(false);
			return {};
		}

		POINT p{};
		if (!ClientToScreen(hWnd, &p)) {
			SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("ClientToScreen 出错"));
			assert(false);
			return {};
		}

		r.bottom += p.y;
		r.left += p.x;
		r.right += p.x;
		r.top += p.y;

		return r;
	}

	static RECT GetScreenRect(HWND hWnd) {
		HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);

		MONITORINFO mi{};
		mi.cbSize = sizeof(mi);
		if (!GetMonitorInfo(hMonitor, &mi)) {
			SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetMonitorInfo 出错"));
			assert(false);
		}
		return mi.rcMonitor;
	}

	static SIZE GetSize(const RECT& rect) {
		return { rect.right - rect.left, rect.bottom - rect.top };
	}

	static D2D1_SIZE_F GetSize(const D2D1_RECT_F& rect) {
		return { rect.right - rect.left,rect.bottom - rect.top };
	}

	// 单位为微秒
	template<typename Fn>
	static int Measure(const Fn& func) {
		using namespace std::chrono;

		auto t = steady_clock::now();
		func();
		auto dura = duration_cast<microseconds>(steady_clock::now() - t);

		return int(dura.count());
	}

	static bool ReadFile(const wchar_t* fileName, std::vector<BYTE>& result);

	static bool ReadTextFile(const wchar_t* fileName, std::string& result);

	static bool CompilePixelShader(std::string_view hlsl, const char* entryPoint, ID3DBlob** blob) {
		ComPtr<ID3DBlob> errorMsgs = nullptr;

		UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
		HRESULT hr = D3DCompile(hlsl.data(), hlsl.size(), nullptr, nullptr, nullptr,
			entryPoint, "ps_5_0", flags, 0, blob, &errorMsgs);
		if (FAILED(hr)) {
			if (errorMsgs) {
				SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg(
					fmt::format("编译像素着色器失败：{}", (const char*)errorMsgs->GetBufferPointer()), hr));
			}
			return false;
		}

		return true;
	}

	static const RTL_OSVERSIONINFOW& GetOSVersion();

	static int CompareVersion(int major1, int minor1, int build1, int major2, int minor2, int build2) {
		if (major1 != major2) {
			return major1 - major2;
		}

		if (minor1 != minor2) {
			return minor1 - minor2;
		} else {
			return build1 - build2;
		}
	}

	template<typename T>
	class ScopeExit {
	public:
		ScopeExit(const ScopeExit&) = delete;
		ScopeExit(ScopeExit&&) = delete;

		explicit ScopeExit(T&& exitScope) : _exitScope(std::forward<T>(exitScope)) {}
		~ScopeExit() { _exitScope(); }

	private:
		T _exitScope;
	};
};
