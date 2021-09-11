#pragma once
#include "pch.h"


class Utils {
public:
	static UINT GetWindowShowCmd(HWND hwnd);

	static bool GetClientScreenRect(HWND hWnd, RECT& rect);

	static RECT GetScreenRect(HWND hWnd);

	static SIZE GetSize(const RECT& rect) {
		return { rect.right - rect.left, rect.bottom - rect.top };
	}

	static D2D1_SIZE_F GetSize(const D2D1_RECT_F& rect) {
		return { rect.right - rect.left,rect.bottom - rect.top };
	}

	static BOOL Str2GUID(const std::wstring_view& szGUID, GUID& guid);

	static std::string GUID2Str(GUID guid);

	static std::wstring UTF8ToUTF16(std::string_view str);

	static std::string UTF16ToUTF8(std::wstring_view str);

	static int Measure(std::function<void()> func);

	static bool ReadFile(const wchar_t* fileName, std::vector<BYTE>& result);

	static bool ReadTextFile(const wchar_t* fileName, std::string& result);

	static bool CompilePixelShader(const char* hlsl, size_t hlslLen, ID3DBlob** blob);
};
