#pragma once
#include "CommonPCH.h"
#include <bcrypt.h>


struct Win32Utils {
	static UINT GetOSBuild();

	static SIZE GetSizeOfRect(const RECT& rect) noexcept {
		return { rect.right - rect.left, rect.bottom - rect.top };
	}

	static bool CheckOverlap(const RECT& r1, const RECT& r2) noexcept {
		return r1.right > r2.left && r1.bottom > r2.top && r1.left < r2.right&& r1.top < r2.bottom;
	}

	static std::wstring GetWndClassName(HWND hWnd);

	static std::wstring GetWndTitle(HWND hWnd);

	static std::wstring GetPathOfWnd(HWND hWnd);

	static bool IsPackaged(HWND hWnd);

	static UINT GetWindowShowCmd(HWND hWnd);

	static bool GetClientScreenRect(HWND hWnd, RECT& rect);

	static bool GetWindowFrameRect(HWND hWnd, RECT& result);

	static bool ReadFile(const wchar_t* fileName, std::vector<BYTE>& result);

	static bool ReadTextFile(const wchar_t* fileName, std::string& result);

	static bool WriteFile(const wchar_t* fileName, const void* buffer, size_t bufferSize);

	static bool WriteTextFile(const wchar_t* fileName, std::string_view text);

	static bool FileExists(const wchar_t* fileName) noexcept {
		DWORD attrs = GetFileAttributes(fileName);
		// 排除文件夹
		return (attrs != INVALID_FILE_ATTRIBUTES) && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
	}

	static bool DirExists(const wchar_t* fileName) noexcept {
		DWORD attrs = GetFileAttributes(fileName);
		return (attrs != INVALID_FILE_ATTRIBUTES) && (attrs & FILE_ATTRIBUTE_DIRECTORY);
	}

	static bool CreateDir(const std::wstring& path, bool recursive = false);

	static const RTL_OSVERSIONINFOW& GetOSVersion() noexcept;

	// CRITICAL_SECTION 的封装，满足基本可锁定要求（BasicLockable）
	// 因此可用于 std::scoped_lock 等
	class CSMutex {
	public:
		CSMutex() noexcept {
			InitializeCriticalSectionEx(&_cs, 4000, CRITICAL_SECTION_NO_DEBUG_INFO);
		}

		~CSMutex() noexcept {
			DeleteCriticalSection(&_cs);
		}

		void lock() noexcept {
			EnterCriticalSection(&_cs);
		}

		void unlock() noexcept {
			LeaveCriticalSection(&_cs);
		}

		CRITICAL_SECTION* get() noexcept {
			return &_cs;
		}
	private:
		CRITICAL_SECTION _cs{};
	};

	class Hasher {
	public:
		static Hasher& Get() noexcept {
			static Hasher instance;
			return instance;
		}

		bool Hash(std::span<const BYTE> data, std::vector<BYTE>& result);

		DWORD GetHashLength() noexcept;
	private:
		~Hasher();

		bool _Initialize();

		CSMutex _cs;	// 同步对 Hash() 的访问

		BCRYPT_ALG_HANDLE _hAlg = NULL;
		DWORD _hashObjLen = 0;		// hash 对象的大小
		void* _hashObj = nullptr;	// 存储 hash 对象
		DWORD _hashLen = 0;			// 哈希结果的大小
		BCRYPT_HASH_HANDLE _hHash = NULL;
	};

	struct HandleCloser { void operator()(HANDLE h) noexcept { assert(h != INVALID_HANDLE_VALUE); if (h) CloseHandle(h); } };

	using ScopedHandle = std::unique_ptr<std::remove_pointer<HANDLE>::type, HandleCloser>;

	static HANDLE SafeHandle(HANDLE h) noexcept { return (h == INVALID_HANDLE_VALUE) ? nullptr : h; }

	// 并行执行 times 次 func，并行失败时回退到单线程
	// 执行完毕后返回
	static void RunParallel(std::function<void(UINT)> func, UINT times);

	static bool IsStartMenu(HWND hWnd);

	// 强制切换前台窗口
	static bool SetForegroundWindow(HWND hWnd);

	// 全局显示/隐藏系统光标
	static bool ShowSystemCursor(bool value);

	// 获取 Virtual Key 的名字
	static std::wstring GetKeyName(DWORD key);
};
