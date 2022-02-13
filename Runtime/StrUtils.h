#pragma once
#include "pch.h"


struct StrUtils {
	static std::wstring UTF8ToUTF16(std::string_view str);

	static std::string UTF16ToUTF8(std::wstring_view str);

	static void Trim(std::string_view& str);

	static std::vector<std::string_view> Split(std::string_view str, char delimiter) {
		std::vector<std::string_view> result;
		while (!str.empty()) {
			size_t pos = str.find(delimiter, 0);
			result.push_back(str.substr(0, pos));

			if (pos == std::string_view::npos) {
				return result;
			} else {
				str.remove_prefix(pos + 1);
			}
		}
		return result;
	}

	static int isspace(char c) {
		return std::isspace(static_cast<unsigned char>(c));
	}

	static int isalpha(char c) {
		return std::isalpha(static_cast<unsigned char>(c));
	}

	static int isalnum(char c) {
		return std::isalnum(static_cast<unsigned char>(c));
	}

	static char toupper(char c) {
		return std::toupper(static_cast<unsigned char>(c));
	}

	static char tolower(char c) {
		return std::tolower(static_cast<unsigned char>(c));
	}

	static std::string ToUpperCase(std::string_view str) {
		std::string result(str);
		ToUpperCase(result);
		return result;
	}

	static void ToUpperCase(std::string& str) {
		std::transform(str.begin(), str.end(), str.begin(), toupper);
	}

	static std::string ToLowerCase(std::string_view str) {
		std::string result(str);
		ToLowerCase(result);
		return result;
	}

	static void ToLowerCase(std::string& str) {
		std::transform(str.begin(), str.end(), str.begin(), tolower);
	}

	template<typename CHAR_T>
	static constexpr size_t StrLen(const CHAR_T* str) {
		// std::char_traits 相比 std::strlen 支持更多字符类型
		// 目前 MSVC 使用 __builtin_strlen，可以在编译时计算字符串常量的长度
		return std::char_traits<CHAR_T>::length(str);
	}
};

