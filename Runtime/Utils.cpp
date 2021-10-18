#include "pch.h"
#include "Utils.h"
#include <io.h>
#include "StrUtils.h"
#include <winternl.h>


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

Utils::MD5::MD5() {
	NTSTATUS status = BCryptOpenAlgorithmProvider(&_hAlg, BCRYPT_MD5_ALGORITHM, NULL, 0);
	if (!NT_SUCCESS(status)) {
		SPDLOG_LOGGER_ERROR(logger,fmt::format("BCryptOpenAlgorithmProvider 失败\n\tNTSTATUS={}", status));
		return;
	}

	ULONG result;

	status = BCryptGetProperty(_hAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&_hashObjLen, sizeof(_hashObjLen), &result, 0);
	if (!NT_SUCCESS(status)) {
		SPDLOG_LOGGER_ERROR(logger, fmt::format("BCryptGetProperty 失败\n\tNTSTATUS={}", status));
		return;
	}

	_hashObj = HeapAlloc(GetProcessHeap(), 0, _hashObjLen);
	if (!_hashObj) {
		SPDLOG_LOGGER_ERROR(logger, "HeapAlloc 失败");
		return;
	}

	status = BCryptGetProperty(_hAlg, BCRYPT_HASH_LENGTH, (PBYTE)&_hashLen, sizeof(_hashLen), &result, 0);
	if (!NT_SUCCESS(status)) {
		SPDLOG_LOGGER_ERROR(logger, fmt::format("BCryptGetProperty 失败\n\tNTSTATUS={}", status));
		return;
	}

	SPDLOG_LOGGER_INFO(logger, "Utils::MD5 初始化成功");
}

Utils::MD5::~MD5() {
	if (_hAlg) {
		BCryptCloseAlgorithmProvider(_hAlg, 0);
	}
	if (_hashObj) {
		HeapFree(GetProcessHeap(), 0, _hashObj);
	}
}


bool Utils::MD5::Hash(void* data, size_t len, std::vector<BYTE>& result) {
	if (_hashLen == 0) {
		// 初始化失败
		return false;
	}
	result.resize(_hashLen);

	BCRYPT_HASH_HANDLE hHash;
	NTSTATUS status = BCryptCreateHash(_hAlg, &hHash, (PUCHAR)_hashObj, _hashObjLen, NULL, 0, 0);
	if (!NT_SUCCESS(status)) {
		SPDLOG_LOGGER_ERROR(logger, fmt::format("BCryptCreateHash 失败\n\tNTSTATUS={}", status));
		return false;
	}

	status = BCryptHashData(hHash, (PUCHAR)data, len, 0);
	if (!NT_SUCCESS(status)) {
		SPDLOG_LOGGER_ERROR(logger, fmt::format("BCryptCreateHash 失败\n\tNTSTATUS={}", status));
		return false;
	}

	status = BCryptFinishHash(hHash, result.data(), result.size(), 0);
	if (!NT_SUCCESS(status)) {
		SPDLOG_LOGGER_ERROR(logger, fmt::format("BCryptFinishHash 失败\n\tNTSTATUS={}", status));
		return false;
	}

	BCryptDestroyHash(hHash);

	return true;
}

