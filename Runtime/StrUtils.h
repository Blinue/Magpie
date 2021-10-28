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

	static std::string ToUpperCase(std::string_view str) {
		std::string result(str);
		std::transform(result.begin(), result.end(), result.begin(), toupper);
		return result;
	}
};

