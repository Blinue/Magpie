#include "pch.h"
#include "Utils.h"

bool Utils::FileExists(const wchar_t* fileName) noexcept {
	DWORD attrs = GetFileAttributes(fileName);
	// 排除文件夹
	return (attrs != INVALID_FILE_ATTRIBUTES) && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

std::string Utils::UTF16ToUTF8(std::wstring_view str) noexcept {
	if (str.empty()) {
		return {};
	}

	int convertResult = WideCharToMultiByte(CP_UTF8, 0, str.data(), (int)str.size(),
		nullptr, 0, nullptr, nullptr);
	if (convertResult <= 0) {
		assert(false);
		return {};
	}

	std::string result(convertResult + 10, L'\0');
	convertResult = WideCharToMultiByte(CP_UTF8, 0, str.data(), (int)str.size(),
		result.data(), (int)result.size(), nullptr, nullptr);
	if (convertResult <= 0) {
		assert(false);
		return {};
	}

	result.resize(convertResult);
	return result;
}

std::vector<std::string_view> Utils::Split(std::string_view str, char delimiter) noexcept {
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
