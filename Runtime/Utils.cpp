#include "pch.h"
#include "Utils.h"
#include <io.h>
#include "StrUtils.h"
#include <winternl.h>


bool Utils::ReadFile(const wchar_t* fileName, std::vector<BYTE>& result) {
	SPDLOG_LOGGER_INFO(logger, fmt::format("读取文件：{}", StrUtils::UTF16ToUTF8(fileName)));

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
	ScopedHandle hFile(SafeHandle(CreateFile2(fileName, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, nullptr)));
#else
	ScopedHandle hFile(SafeHandle(CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr)));
#endif

	if (!hFile) {
		SPDLOG_LOGGER_ERROR(logger, "打开文件失败");
		return false;
	}
	
	DWORD size = GetFileSize(hFile.get(), nullptr);
	result.resize(size);

	DWORD readed;
	if (!::ReadFile(hFile.get(), result.data(), size, &readed, nullptr)) {
		SPDLOG_LOGGER_ERROR(logger, "读取文件失败");
		return false;
	}

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

bool Utils::WriteFile(const wchar_t* fileName, const void* buffer, size_t bufferSize) {
	FILE* hFile;
	if (_wfopen_s(&hFile, fileName, L"wb") || !hFile) {
		SPDLOG_LOGGER_ERROR(logger, fmt::format("打开文件{}失败", StrUtils::UTF16ToUTF8(fileName)));
		return false;
	}

	size_t writed = fwrite(buffer, 1, bufferSize, hFile);
	assert(writed == bufferSize);

	fclose(hFile);
	return true;
}

RTL_OSVERSIONINFOW _GetOSVersion() {
	HMODULE hNtDll = GetModuleHandle(L"ntdll.dll");
	if (!hNtDll) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("获取 ntdll.dll 句柄失败"));
		return {};
	}
	
	auto rtlGetVersion = (LONG(WINAPI*)(PRTL_OSVERSIONINFOW))GetProcAddress(hNtDll, "RtlGetVersion");
	if (rtlGetVersion == nullptr) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("获取 RtlGetVersion 地址失败"));
		assert(false);
		return {};
	}

	RTL_OSVERSIONINFOW version{};
	version.dwOSVersionInfoSize = sizeof(version);
	rtlGetVersion(&version);

	return version;
}

const RTL_OSVERSIONINFOW& Utils::GetOSVersion() {
	static RTL_OSVERSIONINFOW version = _GetOSVersion();
	return version;
}

bool _IsWin10OrNewer() {
	const RTL_OSVERSIONINFOW& osVer = Utils::GetOSVersion();
	return osVer.dwMajorVersion >= 10;
}

bool Utils::IsWin10OrNewer() {
	static bool value = _IsWin10OrNewer();
	return value;
}

std::string Utils::Bin2Hex(BYTE* data, size_t len) {
	if (!data || len == 0) {
		return {};
	}

	static char oct2Hex[16] = {
		'0','1','2','3','4','5','6','7',
		'8','9','a','b','c','d','e','f'
	};

	std::string result(len * 2, 0);
	char* pResult = &result[0];

	for (size_t i = 0; i < len; ++i) {
		BYTE b = *data++;
		*pResult++ = oct2Hex[(b >> 4) & 0xf];
		*pResult++ = oct2Hex[b & 0xf];
	}

	return result;
}


Utils::Hasher::~Hasher() {
	if (_hAlg) {
		BCryptCloseAlgorithmProvider(_hAlg, 0);
	}
	if (_hashObj) {
		HeapFree(GetProcessHeap(), 0, _hashObj);
	}
	if (_supportReuse && _hHash) {
		BCryptDestroyHash(_hHash);
	}
}

bool Utils::Hasher::Initialize() {
	NTSTATUS status = BCryptOpenAlgorithmProvider(&_hAlg, BCRYPT_SHA1_ALGORITHM, NULL, 0);
	if (!NT_SUCCESS(status)) {
		SPDLOG_LOGGER_ERROR(logger, fmt::format("BCryptOpenAlgorithmProvider 失败\n\tNTSTATUS={}", status));
		return false;
	}

	ULONG result;

	status = BCryptGetProperty(_hAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&_hashObjLen, sizeof(_hashObjLen), &result, 0);
	if (!NT_SUCCESS(status)) {
		SPDLOG_LOGGER_ERROR(logger, fmt::format("BCryptGetProperty 失败\n\tNTSTATUS={}", status));
		return false;
	}

	_hashObj = HeapAlloc(GetProcessHeap(), 0, _hashObjLen);
	if (!_hashObj) {
		SPDLOG_LOGGER_ERROR(logger, "HeapAlloc 失败");
		return false;
	}

	status = BCryptGetProperty(_hAlg, BCRYPT_HASH_LENGTH, (PBYTE)&_hashLen, sizeof(_hashLen), &result, 0);
	if (!NT_SUCCESS(status)) {
		SPDLOG_LOGGER_ERROR(logger, fmt::format("BCryptGetProperty 失败\n\tNTSTATUS={}", status));
		return false;
	}

	status = BCryptCreateHash(_hAlg, &_hHash, (PUCHAR)_hashObj, _hashObjLen, NULL, 0, BCRYPT_HASH_REUSABLE_FLAG);
	if (NT_SUCCESS(status)) {
		_supportReuse = true;
	} else {
		SPDLOG_LOGGER_WARN(logger, fmt::format("BCryptCreateHash 失败：当前设备不支持 BCRYPT_HASH_REUSABLE_FLAG\n\tNTSTATUS={}", status));
	}

	SPDLOG_LOGGER_INFO(logger, "Utils::Hasher 初始化成功");
	return true;
}

bool Utils::Hasher::Hash(void* data, size_t len, std::vector<BYTE>& result) {
	result.resize(_hashLen);

	NTSTATUS status;

	if (!_supportReuse) {
		status = BCryptCreateHash(_hAlg, &_hHash, (PUCHAR)_hashObj, _hashObjLen, NULL, 0, BCRYPT_HASH_REUSABLE_FLAG);
		if (!NT_SUCCESS(status)) {
			SPDLOG_LOGGER_ERROR(logger, fmt::format("BCryptCreateHash 失败\n\tNTSTATUS={}", status));
			return false;
		}
	}

	status = BCryptHashData(_hHash, (PUCHAR)data, (ULONG)len, 0);
	if (!NT_SUCCESS(status)) {
		SPDLOG_LOGGER_ERROR(logger, fmt::format("BCryptCreateHash 失败\n\tNTSTATUS={}", status));
		return false;
	}

	status = BCryptFinishHash(_hHash, result.data(), (ULONG)result.size(), 0);
	if (!NT_SUCCESS(status)) {
		SPDLOG_LOGGER_ERROR(logger, fmt::format("BCryptFinishHash 失败\n\tNTSTATUS={}", status));
		return false;
	}

	if (!_supportReuse) {
		BCryptDestroyHash(_hHash);
	}

	return true;
}

