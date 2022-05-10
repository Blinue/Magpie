#include "pch.h"
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

void StrUtils::Trim(std::string_view& str) {
	for (int i = 0; i < str.size(); ++i) {
		if (!isspace(str[i])) {
			str.remove_prefix(i);

			size_t i = str.size() - 1;
			for (; i > 0; --i) {
				if (!isspace(str[i])) {
					break;
				}
			}
			str.remove_suffix(str.size() - 1 - i);
			return;
		}
	}

	str.remove_prefix(str.size());
}
