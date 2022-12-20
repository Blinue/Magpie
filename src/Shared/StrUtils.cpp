#include "pch.h"
#include "StrUtils.h"
#include "Logger.h"

std::wstring StrUtils::UTF8ToUTF16(std::string_view str) noexcept {
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

static std::string UTF16ToOther(UINT codePage, std::wstring_view str) noexcept {
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

std::string StrUtils::UTF16ToUTF8(std::wstring_view str) noexcept {
	return UTF16ToOther(CP_UTF8, str);
}

std::string StrUtils::UTF16ToANSI(std::wstring_view str) noexcept {
	return UTF16ToOther(CP_ACP, str);
}
