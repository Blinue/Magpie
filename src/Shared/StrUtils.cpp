#include "StrUtils.h"
#include "Logger.h"


std::wstring StrUtils::UTF8ToUTF16(std::string_view str) {
	int convertResult = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), nullptr, 0);
	if (convertResult <= 0) {
		Logger::Get().Win32Error("MultiByteToWideChar 失败");
		assert(false);
		return {};
	}

	std::wstring r(convertResult + 10, L'\0');
	convertResult = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), &r[0], (int)r.size());
	if (convertResult <= 0) {
		Logger::Get().Win32Error("MultiByteToWideChar 失败");
		assert(false);
		return {};
	}

	return std::wstring(r.begin(), r.begin() + convertResult);
}

std::string StrUtils::UTF16ToUTF8(std::wstring_view str) {
	int convertResult = WideCharToMultiByte(CP_UTF8, 0, str.data(), (int)str.size(), nullptr, 0, nullptr, nullptr);
	if (convertResult <= 0) {
		Logger::Get().Win32Error("WideCharToMultiByte 失败");
		assert(false);
		return {};
	}

	std::string r(convertResult + 10, L'\0');
	convertResult = WideCharToMultiByte(CP_UTF8, 0, str.data(), (int)str.size(), &r[0], (int)r.size(), nullptr, nullptr);
	if (convertResult <= 0) {
		Logger::Get().Win32Error("WideCharToMultiByte 失败");
		assert(false);
		return {};
	}

	return std::string(r.begin(), r.begin() + convertResult);
}

std::string StrUtils::UTF16ToANSI(std::wstring_view str) {
	int convertResult = WideCharToMultiByte(CP_ACP, 0, str.data(), (int)str.size(), nullptr, 0, nullptr, nullptr);
	if (convertResult <= 0) {
		Logger::Get().Win32Error("WideCharToMultiByte 失败");
		assert(false);
		return {};
	}

	std::string r(convertResult + 10, L'\0');
	convertResult = WideCharToMultiByte(CP_ACP, 0, str.data(), (int)str.size(), &r[0], (int)r.size(), nullptr, nullptr);
	if (convertResult <= 0) {
		Logger::Get().Win32Error("WideCharToMultiByte 失败");
		assert(false);
		return {};
	}

	return std::string(r.begin(), r.begin() + convertResult);
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
