#pragma once
#include "pch.h"
#include <bcrypt.h>


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

	static bool WriteFile(const wchar_t* fileName, const void* buffer, size_t bufferSize);

	static bool FileExists(const wchar_t* fileName) {
		DWORD attrs = GetFileAttributesW(fileName);
		// 排除文件夹
		return (attrs != INVALID_FILE_ATTRIBUTES) && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
	}

	static bool DirExists(const wchar_t* fileName) {
		DWORD attrs = GetFileAttributesW(fileName);
		return (attrs != INVALID_FILE_ATTRIBUTES) && (attrs & FILE_ATTRIBUTE_DIRECTORY);
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

	static bool IsWin10OrNewer();

	static std::string Bin2Hex(BYTE* data, size_t len);

	class Hasher {
	public:
		static Hasher& GetInstance() {
			static Hasher instance;
			return instance;
		}

		bool Initialize();

		bool Hash(void* data, size_t len, std::vector<BYTE>& result);

		DWORD GetHashLength() {
			return _hashLen;
		}
	private:
		~Hasher();

		BCRYPT_ALG_HANDLE _hAlg = NULL;
		DWORD _hashObjLen = 0;		// hash 对象的大小
		void* _hashObj = nullptr;	// 存储 hash 对象
		DWORD _hashLen = 0;			// 哈希结果的大小
		BCRYPT_HASH_HANDLE _hHash = NULL;
		bool _supportReuse = false;
	};

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

	struct HandleCloser { void operator()(HANDLE h) noexcept { assert(h != INVALID_HANDLE_VALUE); if (h) CloseHandle(h); } };

	using ScopedHandle = std::unique_ptr<std::remove_pointer<HANDLE>::type, HandleCloser>;

	static HANDLE SafeHandle(HANDLE h) noexcept { return (h == INVALID_HANDLE_VALUE) ? nullptr : h; }
};
