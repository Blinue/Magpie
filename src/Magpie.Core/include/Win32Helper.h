#pragma once
#include "Version.h"

namespace Magpie {

struct Win32Helper {
	static SIZE GetSizeOfRect(const RECT& rect) noexcept {
		return { rect.right - rect.left, rect.bottom - rect.top };
	}

	static bool CheckOverlap(const RECT& r1, const RECT& r2) noexcept {
		return r1.right > r2.left && r1.bottom > r2.top && r1.left < r2.right && r1.top < r2.bottom;
	}

	static std::wstring GetWndClassName(HWND hWnd) noexcept;

	static std::wstring GetWndTitle(HWND hWnd) noexcept;

	static wil::unique_process_handle GetWndProcessHandle(HWND hWnd) noexcept;

	static std::wstring GetPathOfWnd(HWND hWnd) noexcept;

	static UINT GetWindowShowCmd(HWND hWnd) noexcept;

	static bool GetClientScreenRect(HWND hWnd, RECT& rect) noexcept;

	static bool GetWindowFrameRect(HWND hWnd, RECT& rect) noexcept;

	static uint32_t GetNativeWindowBorderThickness(uint32_t dpi) noexcept;

	static bool ReadFile(const wchar_t* fileName, std::vector<uint8_t>& result) noexcept;

	static bool ReadTextFile(const wchar_t* fileName, std::string& result) noexcept;

	static bool WriteFile(const wchar_t* fileName, const void* buffer, size_t bufferSize) noexcept;

	static bool WriteTextFile(const wchar_t* fileName, std::string_view text) noexcept;

	static bool FileExists(const wchar_t* fileName) noexcept;

	static bool DirExists(const wchar_t* fileName) noexcept;

	// 相比 wil::CreateDirectoryDeepNoThrow 支持相对路径而且更快
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

	// 并行执行 times 次 func，并行失败时回退到单线程
	// 执行完毕后返回
	static void RunParallel(std::function<void(uint32_t)> func, uint32_t times) noexcept;

	// 强制切换前台窗口
	static bool SetForegroundWindow(HWND hWnd) noexcept;

	// 获取 Virtual Key 的名字
	static const std::wstring& GetKeyName(uint8_t key) noexcept;

	static bool IsProcessElevated() noexcept;

	static bool GetProcessIntegrityLevel(HANDLE hQueryToken, DWORD& integrityLevel) noexcept;

	// VARIANT 封装，自动管理生命周期，比 WIL 提供更多功能
	struct Variant : public VARIANT {
		Variant() noexcept {
			VariantInit(this);
		}

		Variant(const Variant& other) noexcept : Variant(static_cast<const VARIANT&>(other)) {}

		Variant(Variant&& other) noexcept : Variant(static_cast<VARIANT&&>(other)) {}

		Variant(const VARIANT& varSrc) noexcept {
			VariantInit(this);
			std::ignore = VariantCopy(this, const_cast<VARIANT*>(&varSrc));
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
			std::ignore = VariantCopy(this, const_cast<VARIANT*>(&other));
			return *this;
		}

		Variant& operator=(VARIANT&& other) noexcept {
			std::memcpy(this, &other, sizeof(other));
			other.vt = VT_EMPTY;
			return *this;
		}
	};

	static bool ShellOpen(const wchar_t* path, const wchar_t* parameters = nullptr, bool nonElevated = true) noexcept;
	// 不应在主线程调用
	static bool OpenFolderAndSelectFile(const wchar_t* fileName) noexcept;

	static const std::wstring& GetExePath() noexcept;
};

}

constexpr bool operator==(const SIZE& l, const SIZE& r) noexcept {
	return l.cx == r.cx && l.cy == r.cy;
}

constexpr bool operator==(const POINT& l, const POINT& r) noexcept {
	return l.x == r.x && l.y == r.y;
}

// 避免和 d3d11.h 中的定义冲突，应在该头文件后包含
#ifndef __d3d11_h__
constexpr bool operator==(const RECT& l, const RECT& r) noexcept {
	return l.left == r.left && l.top == r.top && l.right == r.right && l.bottom == r.bottom;
}
#endif
