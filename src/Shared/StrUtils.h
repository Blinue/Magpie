#pragma once
#include <string>
#include <vector>
#include <cwctype>
#include <wtypes.h>	// BSTR
#include "SmallVector.h"

struct StrUtils {
	static std::wstring UTF8ToUTF16(std::string_view str) noexcept;

	static std::string UTF16ToUTF8(std::wstring_view str) noexcept;

	static std::string UTF16ToANSI(std::wstring_view str) noexcept;

	template<typename CHAR_T>
	static void Trim(std::basic_string_view<CHAR_T>& str) noexcept {
		for (int i = 0; i < str.size(); ++i) {
			if (!isspace(str[i])) {
				str.remove_prefix(i);

				size_t j = str.size() - 1;
				for (; j > 0; --j) {
					if (!isspace(str[j])) {
						break;
					}
				}
				str.remove_suffix(str.size() - 1 - j);
				return;
			}
		}

		str.remove_prefix(str.size());
	}

	static void Trim(std::string_view& str) noexcept {
		Trim<char>(str);
	}

	static void Trim(std::wstring_view& str) noexcept {
		Trim<wchar_t>(str);
	}

	template<typename CHAR_T>
	static void Trim(std::basic_string<CHAR_T>& str) noexcept {
		std::basic_string_view<CHAR_T> sv(str);
		Trim(sv);
		str = sv;
	}

	static void Trim(std::string& str) noexcept {
		Trim<char>(str);
	}

	template<typename CHAR_T>
	static std::basic_string<CHAR_T> Trim(const std::basic_string<CHAR_T>& str) noexcept {
		std::basic_string<CHAR_T> result = str;
		Trim(result);
		return result;
	}

	static std::string Trim(const std::string& str) noexcept {
		return Trim<char>(str);
	}

	template<typename CHAR_T>
	static SmallVector<std::basic_string_view<CHAR_T>> Split(std::basic_string_view<CHAR_T> str, CHAR_T delimiter) noexcept {
		SmallVector<std::basic_string_view<CHAR_T>> result;
		while (!str.empty()) {
			size_t pos = str.find(delimiter, 0);
			result.push_back(str.substr(0, pos));

			if (pos == std::basic_string_view<CHAR_T>::npos) {
				return result;
			} else {
				str.remove_prefix(pos + 1);
			}
		}
		return result;
	}

	static SmallVector<std::string_view> Split(std::string_view str, char delimiter) noexcept {
		return Split<char>(str, delimiter);
	}

	static SmallVector<std::wstring_view> Split(std::wstring_view str, wchar_t delimiter) noexcept {
		return Split<wchar_t>(str, delimiter);
	}

	static bool isspace(char c) noexcept {
		return (bool)std::isspace(static_cast<unsigned char>(c));
	}

	static bool isspace(wchar_t c) noexcept {
		return (bool)std::iswspace(c);
	}

	static bool isalpha(char c) noexcept {
		return (bool)std::isalpha(static_cast<unsigned char>(c));
	}

	static bool isalpha(wchar_t c) noexcept {
		return (bool)std::iswalpha(c);
	}

	static bool isdigit(char c) noexcept {
		return (bool)std::isdigit(static_cast<unsigned char>(c));
	}

	static bool isdigit(wchar_t c) noexcept {
		return (bool)std::isdigit(c);
	}

	static bool isalnum(char c) noexcept {
		return (bool)std::isalnum(static_cast<unsigned char>(c));
	}

	static bool isalnum(wchar_t c) noexcept {
		return (bool)std::iswalnum(c);
	}

	static char toupper(char c) noexcept {
		return (char)std::toupper(static_cast<unsigned char>(c));
	}

	static wchar_t toupper(wchar_t c) noexcept {
		return (wchar_t)std::towupper(c);
	}

	static char tolower(char c) noexcept {
		return (char)std::tolower(static_cast<unsigned char>(c));
	}

	static wchar_t tolower(wchar_t c) noexcept {
		return (wchar_t)std::towlower(c);
	}

	template<typename CHAR_T>
	static std::basic_string<CHAR_T> ToUpperCase(std::basic_string_view<CHAR_T> str) noexcept {
		std::basic_string<CHAR_T> result(str);
		ToUpperCase(result);
		return result;
	}

	template<typename CHAR_T>
	static void ToUpperCase(std::basic_string<CHAR_T>& str) noexcept {
		for (CHAR_T& c : str) {
			c = toupper(c);
		}
	}

	template<typename CHAR_T>
	static std::basic_string<CHAR_T> ToLowerCase(std::basic_string_view<CHAR_T> str) noexcept {
		std::basic_string<CHAR_T> result(str);
		ToLowerCase(result);
		return result;
	}

	template<typename CHAR_T>
	static void ToLowerCase(std::basic_string<CHAR_T>& str) noexcept {
		for (CHAR_T& c : str) {
			c = tolower(c);
		}
	}

	template<typename CHAR_T>
	static constexpr size_t StrLen(const CHAR_T* str) noexcept {
		// std::char_traits 相比 std::strlen 支持更多字符类型
		// 目前 MSVC 使用 __builtin_strlen，可以在编译时计算字符串常量的长度
		return std::char_traits<CHAR_T>::length(str);
	}

	template<typename T1, typename T2, typename... AV,
		typename CHAR_T = std::conditional_t<std::is_constructible_v<std::basic_string_view<char>, T1>, char, wchar_t>>
	static std::basic_string<CHAR_T> Concat(T1&& s1, T2&& s2, AV&&... args) noexcept {
		return _Concat<CHAR_T>(std::forward<T1>(s1), std::forward<T2>(s2), std::forward<AV>(args)...);
	}

private:
	template<typename CHAR_T>
	static std::basic_string<CHAR_T> _Concat(
		const std::basic_string_view<CHAR_T>& s1,
		const std::basic_string_view<CHAR_T>& s2
	) noexcept {
		std::basic_string<CHAR_T> result;
		result.reserve(s1.size() + s2.size());
		result.append(s1).append(s2);
		return result;
	}

	template<typename CHAR_T>
	static std::basic_string<CHAR_T> _Concat(
		const std::basic_string_view<CHAR_T>& s1,
		const std::basic_string_view<CHAR_T>& s2,
		const std::basic_string_view<CHAR_T>& s3
	) noexcept {
		std::basic_string<CHAR_T> result;
		result.reserve(s1.size() + s2.size() + s3.size());
		result.append(s1).append(s2).append(s3);
		return result;
	}

	template<typename CHAR_T>
	static std::basic_string<CHAR_T> _Concat(
		const std::basic_string_view<CHAR_T>& s1,
		const std::basic_string_view<CHAR_T>& s2,
		const std::basic_string_view<CHAR_T>& s3,
		const std::basic_string_view<CHAR_T>& s4
	) noexcept {
		std::basic_string<CHAR_T> result;
		result.reserve(s1.size() + s2.size() + s3.size() + s4.size());
		result.append(s1).append(s2).append(s3).append(s4);
		return result;
	}

	template<typename CHAR_T>
	static std::basic_string<CHAR_T> _Concat(
		const std::basic_string_view<CHAR_T>& s1,
		const std::basic_string_view<CHAR_T>& s2,
		const std::basic_string_view<CHAR_T>& s3,
		const std::basic_string_view<CHAR_T>& s4,
		const std::basic_string_view<CHAR_T>& s5
	) noexcept {
		std::basic_string<CHAR_T> result;
		result.reserve(s1.size() + s2.size() + s3.size() + s4.size() + s5.size());
		result.append(s1).append(s2).append(s3).append(s4).append(s5);
		return result;
	}

	template<typename CHAR_T, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename... AV>
	static std::basic_string<CHAR_T> _Concat(
		T1&& s1,
		T2&& s2,
		T3&& s3,
		T4&& s4,
		T5&& s5,
		T6&& s6,
		AV&&... args
	) noexcept {
		return _ConcatHelper<CHAR_T>({
			std::forward<T1>(s1),
			std::forward<T2>(s2),
			std::forward<T3>(s3),
			std::forward<T4>(s4),
			std::forward<T5>(s5),
			std::forward<T6>(s6),
			std::forward<AV>(args)...
		});
	}

	template<typename CHAR_T>
	static std::basic_string<CHAR_T> _ConcatHelper(std::initializer_list<std::basic_string_view<CHAR_T>> args) noexcept {
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
