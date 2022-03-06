#include "pch.h"
#include "Utils.h"
#include <io.h>
#include <winternl.h>
#include "StrUtils.h"
#include "Logger.h"
#include <zstd.h>


UINT Utils::GetWindowShowCmd(HWND hwnd) {
	assert(hwnd != NULL);

	WINDOWPLACEMENT wp{};
	wp.length = sizeof(wp);
	if (!GetWindowPlacement(hwnd, &wp)) {
		Logger::Get().Win32Error("GetWindowPlacement 出错");
		assert(false);
	}

	return wp.showCmd;
}

bool Utils::GetClientScreenRect(HWND hWnd, RECT& rect) {
	if (!GetClientRect(hWnd, &rect)) {
		Logger::Get().Win32Error("GetClientRect 出错");
		return false;
	}

	POINT p{};
	if (!ClientToScreen(hWnd, &p)) {
		Logger::Get().Win32Error("ClientToScreen 出错");
		return false;
	}

	rect.bottom += p.y;
	rect.left += p.x;
	rect.right += p.x;
	rect.top += p.y;

	return true;
}

bool Utils::GetWindowFrameRect(HWND hWnd, RECT& result) {
	HRESULT hr = DwmGetWindowAttribute(hWnd,
		DWMWA_EXTENDED_FRAME_BOUNDS, &result, sizeof(result));
	if (FAILED(hr)) {
		Logger::Get().ComError("DwmGetWindowAttribute 失败", hr);
		return false;
	}

	return true;
}

bool Utils::ReadFile(const wchar_t* fileName, std::vector<BYTE>& result) {
	Logger::Get().Info(StrUtils::Concat("读取文件：", StrUtils::UTF16ToUTF8(fileName)));

	CREATEFILE2_EXTENDED_PARAMETERS extendedParams = {};
	extendedParams.dwSize = sizeof(CREATEFILE2_EXTENDED_PARAMETERS);
	extendedParams.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
	extendedParams.dwFileFlags = FILE_FLAG_SEQUENTIAL_SCAN;
	extendedParams.dwSecurityQosFlags = SECURITY_ANONYMOUS;
	extendedParams.lpSecurityAttributes = nullptr;
	extendedParams.hTemplateFile = nullptr;

	ScopedHandle hFile(SafeHandle(CreateFile2(fileName, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, &extendedParams)));

	if (!hFile) {
		Logger::Get().Error("打开文件失败");
		return false;
	}
	
	DWORD size = GetFileSize(hFile.get(), nullptr);
	result.resize(size);

	DWORD readed;
	if (!::ReadFile(hFile.get(), result.data(), size, &readed, nullptr)) {
		Logger::Get().Error("读取文件失败");
		return false;
	}

	return true;
}

bool Utils::ReadTextFile(const wchar_t* fileName, std::string& result) {
	FILE* hFile;
	if (_wfopen_s(&hFile, fileName, L"rt") || !hFile) {
		Logger::Get().Error(StrUtils::Concat("打开文件 ", StrUtils::UTF16ToUTF8(fileName), " 失败"));
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
		Logger::Get().Error(StrUtils::Concat("打开文件 ", StrUtils::UTF16ToUTF8(fileName), " 失败"));
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
		Logger::Get().Win32Error("获取 ntdll.dll 句柄失败");
		return {};
	}
	
	auto rtlGetVersion = (LONG(WINAPI*)(PRTL_OSVERSIONINFOW))GetProcAddress(hNtDll, "RtlGetVersion");
	if (rtlGetVersion == nullptr) {
		Logger::Get().Win32Error("获取 RtlGetVersion 地址失败");
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

std::string Utils::Bin2Hex(std::span<const BYTE> data) {
	if (data.size() == 0) {
		return {};
	}

	static char oct2Hex[16] = {
		'0','1','2','3','4','5','6','7',
		'8','9','a','b','c','d','e','f'
	};

	std::string result(data.size() * 2, 0);
	char* pResult = &result[0];

	for (BYTE b : data) {
		*pResult++ = oct2Hex[(b >> 4) & 0xf];
		*pResult++ = oct2Hex[b & 0xf];
	}

	return result;
}


struct TPContext {
	std::function<void(UINT)> func;
	std::atomic<UINT> id;
};

static void CALLBACK TPCallback(PTP_CALLBACK_INSTANCE, PVOID context, PTP_WORK) {
	TPContext* ctxt = (TPContext*)context;
	UINT id = ++ctxt->id;
	ctxt->func(id);
}

void Utils::RunParallel(std::function<void(UINT)> func, UINT times) {
#ifdef _DEBUG
	// 为了便于调试，DEBUG 模式下不使用线程池
	for (UINT i = 0; i < times; ++i) {
		func(i);
	}
#else
	if (times == 0) {
		return;
}

	if (times == 1) {
		return func(0);
	}

	TPContext ctxt = { func, 0 };
	PTP_WORK work = CreateThreadpoolWork(TPCallback, &ctxt, nullptr);
	if (work) {
		// 在线程池中执行 times - 1 次
		for (UINT i = 1; i < times; ++i) {
			SubmitThreadpoolWork(work);
		}

		func(0);

		WaitForThreadpoolWorkCallbacks(work, FALSE);
		CloseThreadpoolWork(work);
	} else {
		Logger::Get().Win32Error("CreateThreadpoolWork 失败，回退到单线程");

		// 回退到单线程
		for (UINT i = 0; i < times; ++i) {
			func(i);
		}
	}
#endif // _DEBUG
}

bool Utils::ZstdCompress(std::span<const BYTE> src, std::vector<BYTE>& dest, int compressionLevel) {
	dest.resize(ZSTD_compressBound(src.size()));
	size_t size = ZSTD_compress(dest.data(), dest.size(), src.data(), src.size(), compressionLevel);

	if (ZSTD_isError(size)) {
		Logger::Get().Error(StrUtils::Concat("压缩失败：", ZSTD_getErrorName(size)));
		return false;
	}

	dest.resize(size);
	return true;
}

bool Utils::ZstdDecompress(std::span<const BYTE> src, std::vector<BYTE>& dest) {
	auto size = ZSTD_getFrameContentSize(src.data(), src.size());
	if (size == ZSTD_CONTENTSIZE_UNKNOWN || size == ZSTD_CONTENTSIZE_ERROR) {
		Logger::Get().Error("ZSTD_getFrameContentSize 失败");
		return false;
	}

	dest.resize(size);
	size = ZSTD_decompress(dest.data(), dest.size(), src.data(), src.size());
	if (ZSTD_isError(size)) {
		Logger::Get().Error(StrUtils::Concat("解压失败：", ZSTD_getErrorName(size)));
		return false;
	}

	dest.resize(size);

	return true;
}


Utils::Hasher::~Hasher() {
	if (_hAlg) {
		BCryptCloseAlgorithmProvider(_hAlg, 0);
	}
	if (_hashObj) {
		HeapFree(GetProcessHeap(), 0, _hashObj);
	}
	if (_hHash) {
		BCryptDestroyHash(_hHash);
	}
}

bool Utils::Hasher::Initialize() {
	NTSTATUS status = BCryptOpenAlgorithmProvider(&_hAlg, BCRYPT_SHA1_ALGORITHM, NULL, 0);
	if (!NT_SUCCESS(status)) {
		Logger::Get().Error(fmt::format("BCryptOpenAlgorithmProvider 失败\n\tNTSTATUS={}", status));
		return false;
	}

	ULONG result;

	status = BCryptGetProperty(_hAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&_hashObjLen, sizeof(_hashObjLen), &result, 0);
	if (!NT_SUCCESS(status)) {
		Logger::Get().Error(fmt::format("BCryptGetProperty 失败\n\tNTSTATUS={}", status));
		return false;
	}

	_hashObj = HeapAlloc(GetProcessHeap(), 0, _hashObjLen);
	if (!_hashObj) {
		Logger::Get().Error("HeapAlloc 失败");
		return false;
	}

	status = BCryptGetProperty(_hAlg, BCRYPT_HASH_LENGTH, (PBYTE)&_hashLen, sizeof(_hashLen), &result, 0);
	if (!NT_SUCCESS(status)) {
		Logger::Get().Error(fmt::format("BCryptGetProperty 失败\n\tNTSTATUS={}", status));
		return false;
	}

	status = BCryptCreateHash(_hAlg, &_hHash, (PUCHAR)_hashObj, _hashObjLen, NULL, 0, BCRYPT_HASH_REUSABLE_FLAG);
	if (!NT_SUCCESS(status)) {
		Logger::Get().Error(fmt::format("BCryptCreateHash 失败\n\tNTSTATUS={}", status));
		return false;
	}

	Logger::Get().Info("Utils::Hasher 初始化成功");
	return true;
}

bool Utils::Hasher::Hash(std::span<const BYTE> data, std::vector<BYTE>& result) {
	// BCrypt API 内部保存状态，因此需要同步对它们访问
	std::scoped_lock lk(_cs);

	result.resize(_hashLen);

	NTSTATUS status = BCryptHashData(_hHash, (PUCHAR)data.data(), (ULONG)data.size(), 0);
	if (!NT_SUCCESS(status)) {
		Logger::Get().Error(fmt::format("BCryptCreateHash 失败\n\tNTSTATUS={}", status));
		return false;
	}

	status = BCryptFinishHash(_hHash, result.data(), (ULONG)result.size(), 0);
	if (!NT_SUCCESS(status)) {
		Logger::Get().Error(fmt::format("BCryptFinishHash 失败\n\tNTSTATUS={}", status));
		return false;
	}

	return true;
}
