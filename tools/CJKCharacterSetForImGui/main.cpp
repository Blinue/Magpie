#include <SDKDDKVer.h>
#include <Windows.h>
#include <iostream>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>
#include <cassert>

struct HandleCloser { void operator()(HANDLE h) noexcept { assert(h != INVALID_HANDLE_VALUE); if (h) CloseHandle(h); } };
using ScopedHandle = std::unique_ptr<std::remove_pointer<HANDLE>::type, HandleCloser>;

static HANDLE SafeHandle(HANDLE h) noexcept {
	return (h == INVALID_HANDLE_VALUE) ? nullptr : h;
}

static std::vector<BYTE> ReadFile(const wchar_t* fileName) noexcept {
	CREATEFILE2_EXTENDED_PARAMETERS extendedParams = {};
	extendedParams.dwSize = sizeof(CREATEFILE2_EXTENDED_PARAMETERS);
	extendedParams.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
	extendedParams.dwFileFlags = FILE_FLAG_SEQUENTIAL_SCAN;
	extendedParams.dwSecurityQosFlags = SECURITY_ANONYMOUS;
	extendedParams.lpSecurityAttributes = nullptr;
	extendedParams.hTemplateFile = nullptr;

	ScopedHandle hFile(SafeHandle(CreateFile2(fileName, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, &extendedParams)));

	if (!hFile) {
		return {};
	}

	DWORD size = GetFileSize(hFile.get(), nullptr);
	std::vector<BYTE> result(size, 0);

	DWORD readed;
	if (!::ReadFile(hFile.get(), result.data(), size, &readed, nullptr)) {
		return {};
	}

	return result;
}

static std::wstring UTF8ToUTF16(std::string_view str) noexcept {
	if (str.empty()) {
		return {};
	}

	int convertResult = MultiByteToWideChar(CP_UTF8, 0,
		str.data(), (int)str.size(), nullptr, 0);
	if (convertResult <= 0) {
		assert(false);
		return {};
	}

	std::wstring result(convertResult + 10, L'\0');
	convertResult = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(),
		result.data(), (int)result.size());
	if (convertResult <= 0) {
		assert(false);
		return {};
	}

	result.resize(convertResult);
	return result;
}

// 输入: input.txt
// 将输出输入文件中所有汉字组成的字符表
int main() {
	std::vector<BYTE> input = ReadFile(L"input.txt");
	input.push_back(0);

	std::wstring utf16 = UTF8ToUTF16(std::string_view((const char*)input.data(), input.size() - 1));

	static constexpr std::pair<wchar_t, wchar_t> CJK_RANGE{ 0x4E00, 0x9FAF };

	std::vector<bool> bitSet(CJK_RANGE.second - CJK_RANGE.first + 1);
	for (wchar_t character : utf16) {
		if (character < CJK_RANGE.first || character > CJK_RANGE.second) {
			continue;
		}

		bitSet[character - CJK_RANGE.first] = true;
	}

	std::vector<uint16_t> index;
	int prevIdx = 0;
	for (int i = 0; i < CJK_RANGE.second - CJK_RANGE.first + 1; ++i) {
		if (bitSet[i]) {
			index.push_back(uint16_t(i - prevIdx));
			prevIdx = i;
		}
	}

	if (index.empty()) {
		return 0;
	}

	std::string out = std::to_string(index[0]);
	for (int i = 1; i < index.size(); ++i) {
		out += ',';
		out += std::to_string(index[i]);
	}

	std::cout << out;
}
