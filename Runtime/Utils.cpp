#include "pch.h"
#include "Utils.h"
#include <io.h>
#include "StrUtils.h"



bool Utils::ReadFile(const wchar_t* fileName, std::vector<BYTE>& result) {
	FILE* hFile;
	if (_wfopen_s(&hFile, fileName, L"rb") || !hFile) {
		SPDLOG_LOGGER_ERROR(logger, fmt::format("打开文件{}失败", StrUtils::UTF16ToUTF8(fileName)));
		return false;
	}

	// 获取文件长度
	int fd = _fileno(hFile);
	long size = _filelength(fd);

	result.resize(size);

	size_t readed = fread(result.data(), 1, size, hFile);
	assert(readed == size);

	fclose(hFile);
	return true;
}

bool Utils::ReadTextFile(const wchar_t* fileName, std::string& result) {
	FILE* hFile;
	if (_wfopen_s(&hFile, fileName, L"rt") || !hFile) {
		SPDLOG_LOGGER_ERROR(logger, fmt::format("打开文件{}失败", StrUtils::UTF16ToUTF8(fileName)));
		return false;
	}

	// 获取文件长度
	int fd = _fileno(hFile);
	long size = _filelength(fd);

	result.clear();
	result.resize(static_cast<size_t>(size) + 1, 0);

	size_t readed = fread(result.data(), 1, size, hFile);
	result.resize(readed);

	fclose(hFile);
	return true;
}

const RTL_OSVERSIONINFOW& Utils::GetOSVersion() {
	static RTL_OSVERSIONINFOW version{};

	if (version.dwMajorVersion == 0) {
		HMODULE hNtDll = LoadLibrary(L"ntdll.dll");
		if (hNtDll == NULL) {
			SPDLOG_LOGGER_CRITICAL(logger, MakeWin32ErrorMsg("加载 ntdll.dll 失败"));
			assert(false);
		}

		auto rtlGetVersion = (LONG(WINAPI*)(PRTL_OSVERSIONINFOW))GetProcAddress(hNtDll, "RtlGetVersion");
		if (rtlGetVersion == nullptr) {
			SPDLOG_LOGGER_CRITICAL(logger, MakeWin32ErrorMsg("获取 RtlGetVersion 地址失败"));
			assert(false);
		}

		rtlGetVersion(&version);

		FreeLibrary(hNtDll);
	}
	
	return version;
}
