#pragma once
#include "Version.h"

struct Win32Utils {
	static SIZE GetSizeOfRect(const RECT& rect) noexcept {
		return { rect.right - rect.left, rect.bottom - rect.top };
	}

	static bool CheckOverlap(const RECT& r1, const RECT& r2) noexcept {
		return r1.right > r2.left && r1.bottom > r2.top && r1.left < r2.right&& r1.top < r2.bottom;
	}

	static std::wstring GetWndClassName(HWND hWnd) noexcept;

	static std::wstring GetWndTitle(HWND hWnd) noexcept;

	static std::wstring GetPathOfWnd(HWND hWnd) noexcept;

	static UINT GetWindowShowCmd(HWND hWnd) noexcept;

	static bool GetClientScreenRect(HWND hWnd, RECT& rect) noexcept;

	static bool GetWindowFrameRect(HWND hWnd, RECT& rect) noexcept;

	static bool IsWindowVisible(HWND hWnd) noexcept;

	static bool ReadFile(const wchar_t* fileName, std::vector<BYTE>& result) noexcept;

	static bool ReadTextFile(const wchar_t* fileName, std::string& result) noexcept;

	static bool WriteFile(const wchar_t* fileName, const void* buffer, size_t bufferSize) noexcept;

	static bool WriteTextFile(const wchar_t* fileName, std::string_view text) noexcept;

	static bool FileExists(const wchar_t* fileName) noexcept {
		DWORD attrs = GetFileAttributes(fileName);
		// 排除文件夹
		return (attrs != INVALID_FILE_ATTRIBUTES) && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
	}

	static bool DirExists(const wchar_t* fileName) noexcept {
		DWORD attrs = GetFileAttributes(fileName);
		return (attrs != INVALID_FILE_ATTRIBUTES) && (attrs & FILE_ATTRIBUTE_DIRECTORY);
	}

	static bool CreateDir(const std::wstring& path, bool recursive = false) noexcept;

	struct OSVersion : Version {
		constexpr OSVersion() {}
		constexpr OSVersion(uint32_t major, uint32_t minor, uint32_t patch)
			: Version(major, minor, patch) {}

		bool Is20H1OrNewer() const noexcept {
			return *this >= Version(10, 0, 19041);
		}

		// 下面为 Win11
		// 不考虑代号相同的 Win10

		bool IsWin11() const noexcept {
			return Is21H2OrNewer();
		}

		bool Is21H2OrNewer() const noexcept {
			return *this >= Version(10, 0, 22000);
		}

		bool Is22H2OrNewer() const noexcept {
			return *this >= Version(10, 0, 22621);
		}
	};

	static const OSVersion& GetOSVersion() noexcept;

	// CRITICAL_SECTION 的封装，满足基本可锁定要求（BasicLockable）
	class CSMutex {
	public:
		CSMutex() noexcept {
			InitializeCriticalSectionEx(&_cs, 4000, CRITICAL_SECTION_NO_DEBUG_INFO);
		}

		CSMutex(const CSMutex&) = delete;
		CSMutex(CSMutex&&) = default;

		~CSMutex() noexcept {
			DeleteCriticalSection(&_cs);
		}

		void lock() noexcept {
			EnterCriticalSection(&_cs);
		}

		void unlock() noexcept {
			LeaveCriticalSection(&_cs);
		}

		CRITICAL_SECTION& get() noexcept {
			return _cs;
		}
	private:
		CRITICAL_SECTION _cs{};
	};

	// SRWLOCK 的封装，满足基本可锁定要求（BasicLockable）
	class SRWMutex {
	public:
		SRWMutex() = default;
		SRWMutex(const SRWMutex&) = delete;
		SRWMutex(SRWMutex&&) = default;

		void lock() noexcept {
			lock_exclusive();
		}

		_Requires_lock_held_(_srwLock)
		void unlock() noexcept {
			unlock_exclusive();
		}

		void lock_exclusive() noexcept {
			AcquireSRWLockExclusive(&_srwLock);
		}

		_Requires_lock_held_(_srwLock)
		void unlock_exclusive() noexcept {
			ReleaseSRWLockExclusive(&_srwLock);
		}

		void lock_shared() noexcept {
			AcquireSRWLockShared(&_srwLock);
		}

		void unlock_shared() noexcept {
			ReleaseSRWLockShared(&_srwLock);
		}

		SRWLOCK& get() noexcept {
			return _srwLock;
		}
	private:
		SRWLOCK _srwLock = SRWLOCK_INIT;
	};

	struct HandleCloser { void operator()(HANDLE h) noexcept { assert(h != INVALID_HANDLE_VALUE); if (h) CloseHandle(h); } };

	using ScopedHandle = std::unique_ptr<std::remove_pointer<HANDLE>::type, HandleCloser>;

	static HANDLE SafeHandle(HANDLE h) noexcept { return (h == INVALID_HANDLE_VALUE) ? nullptr : h; }

	// 并行执行 times 次 func，并行失败时回退到单线程
	// 执行完毕后返回
	static void RunParallel(std::function<void(uint32_t)> func, uint32_t times) noexcept;

	// 强制切换前台窗口
	static bool SetForegroundWindow(HWND hWnd) noexcept;

	// 获取 Virtual Key 的名字
	static const std::wstring& GetKeyName(uint8_t key);

	static bool IsProcessElevated() noexcept;

	// VARIANT 封装，自动管理生命周期
	struct Variant : public VARIANT {
		Variant() noexcept {
			VariantInit(this);
		}

		Variant(const Variant& other) noexcept : Variant(static_cast<const VARIANT&>(other)) {}

		Variant(Variant&& other) noexcept : Variant(static_cast<VARIANT&&>(other)) {}

		Variant(const VARIANT& varSrc) noexcept {
			VariantInit(this);
			[[maybe_unused]]
			HRESULT _ = VariantCopy(this, const_cast<VARIANT*>(&varSrc));
		}

		Variant(VARIANT&& varSrc) noexcept {
			std::memcpy(this, &varSrc, sizeof(varSrc));
			varSrc.vt = VT_EMPTY;
		}

		Variant(int value) noexcept {
			VariantInit(this);
			vt = VT_I4;
			intVal = value;
		}

		Variant(std::wstring_view str) noexcept {
			vt = VT_BSTR;
			bstrVal = SysAllocStringLen(str.data(), (UINT)str.size());
		}

		~Variant() noexcept {
			VariantClear(this);
		}

		Variant& operator=(const Variant& other) noexcept {
			return operator=(static_cast<const VARIANT&>(other));
		}

		Variant& operator=(Variant&& other) noexcept {
			return operator=(static_cast<VARIANT&&>(other));
		}

		Variant& operator=(const VARIANT& other) noexcept {
			[[maybe_unused]]
			HRESULT _ = VariantCopy(this, const_cast<VARIANT*>(&other));
			return *this;
		}

		Variant& operator=(VARIANT&& other) noexcept {
			std::memcpy(this, &other, sizeof(other));
			other.vt = VT_EMPTY;
			return *this;
		}
	};

	// 简单的 BSTR 包装器，用于管理生命周期
	struct BStr {
		BStr() = default;
		BStr(std::wstring_view str);

		BStr(const BStr& other);
		BStr(BStr&& other) noexcept {
			_str = other._str;
			other._str = NULL;
		}

		~BStr();

		BStr& operator=(const BStr& other);
		BStr& operator=(BStr&& other);

		std::wstring ToString() const {
			if (_str) {
				return _str;
			} else {
				return {};
			}
		}

		std::string ToUTF8() const;

		BSTR& Raw() {
			return _str;
		}

		operator BSTR() const {
			return _str;
		}

	private:
		void _Release();

		BSTR _str = NULL;
	};

	static bool ShellOpen(const wchar_t* path, const wchar_t* parameters = nullptr, bool nonElevated = true);
	// 不应在主线程调用
	static bool OpenFolderAndSelectFile(const wchar_t* fileName);
};

constexpr bool operator==(const SIZE& l, const SIZE& r) noexcept {
	return l.cx == r.cx && l.cy == r.cy;
}

constexpr bool operator==(const POINT& l, const POINT& r) noexcept {
	return l.x == r.x && l.y == r.y;
}
