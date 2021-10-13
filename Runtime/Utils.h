#pragma once
#include "pch.h"


class Utils {
public:
	static UINT GetWindowShowCmd(HWND hwnd);

	static RECT GetClientScreenRect(HWND hWnd);

	static RECT GetScreenRect(HWND hWnd);

	static SIZE GetSize(const RECT& rect) {
		return { rect.right - rect.left, rect.bottom - rect.top };
	}

	static D2D1_SIZE_F GetSize(const D2D1_RECT_F& rect) {
		return { rect.right - rect.left,rect.bottom - rect.top };
	}

	static std::wstring UTF8ToUTF16(std::string_view str);

	static std::string UTF16ToUTF8(std::wstring_view str);

	static int Measure(std::function<void()> func);

	static bool ReadFile(const wchar_t* fileName, std::vector<BYTE>& result);

	static bool ReadTextFile(const wchar_t* fileName, std::string& result);

	static bool CompilePixelShader(const char* hlsl, size_t hlslLen, ID3DBlob** blob);

	static const RTL_OSVERSIONINFOW& GetOSVersion();

	static int CompareVersion(int major1, int minor1, int build1, int major2, int minor2, int build2);

	static std::string ToUpperCase(std::string_view str) {
		std::string result(str);
		std::transform(result.begin(), result.end(), result.begin(), toupper);
		return result;
	}

	static void Trim(std::string_view& str);

	static int isspace(char c) {
		return c <= 0 ? 0 : std::isspace(c);
	}

	static int isalpha(char c) {
		return c <= 0 ? 0 : std::isalpha(c);
	}

	static int isalnum(char c) {
		return c <= 0 ? 0 : std::isalnum(c);
	}

	static char toupper(char c) {
		return c <= 0 ? c : std::toupper(c);
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
