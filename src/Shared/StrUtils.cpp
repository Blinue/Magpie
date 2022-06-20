#include "pch.h"
#include "StrUtils.h"
#include "Logger.h"


std::wstring StrUtils::UTF8ToUTF16(std::string_view str) {
	if (str.empty()) {
		return {};
	}

	int convertResult = MultiByteToWideChar(CP_UTF8, 0,
		str.data(), (int)str.size(), nullptr, 0);
	if (convertResult <= 0) {
		Logger::Get().Win32Error("MultiByteToWideChar 失败");
		assert(false);
		return {};
	}

	std::wstring result(convertResult + 10, L'\0');
	convertResult = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(),
		result.data(), (int)result.size());
	if (convertResult <= 0) {
		Logger::Get().Win32Error("MultiByteToWideChar 失败");
		assert(false);
		return {};
	}

	result.resize(convertResult);
	return result;
}

static std::string UTF16ToOther(UINT codePage, std::wstring_view str) {
	if (str.empty()) {
		return {};
	}

	int convertResult = WideCharToMultiByte(codePage, 0, str.data(), (int)str.size(),
		nullptr, 0, nullptr, nullptr);
	if (convertResult <= 0) {
		Logger::Get().Win32Error("WideCharToMultiByte 失败");
		assert(false);
		return {};
	}

	std::string result(convertResult + 10, L'\0');
	convertResult = WideCharToMultiByte(codePage, 0, str.data(), (int)str.size(),
		result.data(), (int)result.size(), nullptr, nullptr);
	if (convertResult <= 0) {
		Logger::Get().Win32Error("WideCharToMultiByte 失败");
		assert(false);
		return {};
	}

	result.resize(convertResult);
	return result;
}

std::string StrUtils::UTF16ToUTF8(std::wstring_view str) {
	return UTF16ToOther(CP_UTF8, str);
}

std::string StrUtils::UTF16ToANSI(std::wstring_view str) {
	return UTF16ToOther(CP_ACP, str);
}

void StrUtils::Trim(std::string_view& str) {
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

std::vector<std::string_view> StrUtils::Split(std::string_view str, char delimiter) {
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
