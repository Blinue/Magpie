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

	template<typename... AV>
	static std::string Concat(std::string_view s1, std::string_view s2, const AV&... args){
		return _Concat(s1, s2, static_cast<const std::string_view&>(args)...);
	}

	template<typename... AV>
	static std::wstring ConcatW(std::wstring_view s1, std::wstring_view s2, const AV&... args) {
		return _Concat(s1, s2, static_cast<const std::wstring_view&>(args)...);
	}

private:
	template<typename CHAR_T>
	static std::basic_string<CHAR_T> _Concat(std::basic_string_view<CHAR_T> s1, std::basic_string_view<CHAR_T> s2) {
		std::basic_string<CHAR_T> result;
		result.reserve(s1.size() + s2.size());
		result.append(s1).append(s2);
		return result;
	}

	template<typename CHAR_T>
	static std::basic_string<CHAR_T> _Concat(
		std::basic_string_view<CHAR_T> s1,
		std::basic_string_view<CHAR_T> s2,
		std::basic_string_view<CHAR_T> s3
	) {
		std::basic_string<CHAR_T> result;
		result.reserve(s1.size() + s2.size() + s3.size());
		result.append(s1).append(s2).append(s3);
		return result;
	}

	template<typename CHAR_T>
	static std::basic_string<CHAR_T> _Concat(
		std::basic_string_view<CHAR_T> s1,
		std::basic_string_view<CHAR_T> s2,
		std::basic_string_view<CHAR_T> s3,
		std::basic_string_view<CHAR_T> s4
	) {
		std::basic_string<CHAR_T> result;
		result.reserve(s1.size() + s2.size() + s3.size() + s4.size());
		result.append(s1).append(s2).append(s3).append(s4);
		return result;
	}

	template<typename CHAR_T>
	static std::basic_string<CHAR_T> _Concat(
		std::basic_string_view<CHAR_T> s1,
		std::basic_string_view<CHAR_T> s2,
		std::basic_string_view<CHAR_T> s3,
		std::basic_string_view<CHAR_T> s4,
		std::basic_string_view<CHAR_T> s5
	) {
		std::basic_string<CHAR_T> result;
		result.reserve(s1.size() + s2.size() + s3.size() + s4.size() + s5.size());
		result.append(s1).append(s2).append(s3).append(s4).append(s5);
		return result;
	}

	template<typename CHAR_T, typename... AV>
	static std::basic_string<CHAR_T> _Concat(
		std::basic_string_view<CHAR_T> s1,
		std::basic_string_view<CHAR_T> s2,
		std::basic_string_view<CHAR_T> s3,
		std::basic_string_view<CHAR_T> s4,
		std::basic_string_view<CHAR_T> s5,
		const AV&... args
	) {
		return _Concat({ s1, s2, s3, s4, s5, static_cast<const std::basic_string_view<CHAR_T>&>(args)... });
	}

	template<typename CHAR_T>
	static std::basic_string<CHAR_T> _Concat(std::initializer_list<std::basic_string_view<CHAR_T>> args) {
		std::basic_string<CHAR_T> result;
		size_t size = 0;
		for (const std::basic_string_view<CHAR_T>& s : args) {
			size += s.size();
		}

		result.reserve(size);

		for (const std::basic_string_view<CHAR_T>& s : args) {
			result.append(s);
		}

		return result;
	}
};

