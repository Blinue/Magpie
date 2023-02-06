#pragma once

struct Utils {
	static bool FileExists(const wchar_t* fileName) noexcept;

	static std::string UTF16ToUTF8(std::wstring_view str) noexcept;

	template<typename CHAR_T>
	static constexpr size_t StrLen(const CHAR_T* str) noexcept {
		// std::char_traits 相比 std::strlen 支持更多字符类型
		// 目前 MSVC 使用 __builtin_strlen，可以在编译时计算字符串常量的长度
		return std::char_traits<CHAR_T>::length(str);
	}

	static std::vector<std::string_view> Split(std::string_view str, char delimiter) noexcept;
};
