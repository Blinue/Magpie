#include "pch.h"
#include "Utils.h"
#include <io.h>


extern std::shared_ptr<spdlog::logger> logger;

UINT Utils::GetWindowShowCmd(HWND hwnd) {
	assert(hwnd != NULL);

	WINDOWPLACEMENT wp{};
	wp.length = sizeof(wp);
	GetWindowPlacement(hwnd, &wp);

	return wp.showCmd;
}

bool Utils::GetClientScreenRect(HWND hWnd, RECT& rect) {
	RECT r;
	if (!GetClientRect(hWnd, &r)) {
		SPDLOG_LOGGER_ERROR(logger,
			fmt::format("GetClientRect 出错\n\tLastErrorCode：{}", GetLastError()));
		return false;
	}

	POINT p{};
	if (!ClientToScreen(hWnd, &p)) {
		SPDLOG_LOGGER_ERROR(logger,
			fmt::format("ClientToScree 出错\n\tLastErrorCode：{}", GetLastError()));
		return false;
	}

	rect.bottom = r.bottom + p.y;
	rect.left = r.left + p.x;
	rect.right = r.right + p.x;
	rect.top = r.top + p.y;

	return true;
}

RECT Utils::GetScreenRect(HWND hWnd) {
	HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);

	MONITORINFO mi{};
	mi.cbSize = sizeof(mi);
	GetMonitorInfo(hMonitor, &mi);
	return mi.rcMonitor;
}

BOOL Utils::Str2GUID(const std::wstring_view& szGUID, GUID& guid) {
	if (szGUID.size() != 36) {
		return FALSE;
	}

	return swscanf_s(szGUID.data(),
		L"%08lx-%04hx-%04hx-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
		&guid.Data1,
		&guid.Data2,
		&guid.Data3,
		&guid.Data4[0], &guid.Data4[1],
		&guid.Data4[2], &guid.Data4[3], &guid.Data4[4], &guid.Data4[5], &guid.Data4[6], &guid.Data4[7]
	) == 11;
}

std::string Utils::GUID2Str(GUID guid) {
	char buf[65]{};

	sprintf_s(buf, "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
		guid.Data1,
		guid.Data2,
		guid.Data3,
		guid.Data4[0], guid.Data4[1],
		guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]
	);
	return { buf };
}

std::wstring Utils::UTF8ToUTF16(std::string_view str) {
	int convertResult = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), nullptr, 0);
	if (convertResult <= 0) {
		SPDLOG_LOGGER_ERROR(logger, "UTF8ToUTF16 失败");
		assert(false);
		return {};
	}

	std::wstring r(convertResult + 10, L'\0');
	convertResult = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), &r[0], (int)r.size());
	if (convertResult <= 0) {
		SPDLOG_LOGGER_ERROR(logger, "UTF8ToUTF16 失败");
		assert(false);
		return {};
	}

	return std::wstring(r.begin(), r.begin() + convertResult);
}

std::string Utils::UTF16ToUTF8(std::wstring_view str) {
	int convertResult = WideCharToMultiByte(CP_UTF8, 0, str.data(), (int)str.size(), nullptr, 0, nullptr, nullptr);
	if (convertResult <= 0) {
		SPDLOG_LOGGER_ERROR(logger, "UTF16ToUTF8 失败");
		assert(false);
		return {};
	}

	std::string r(convertResult + 10, L'\0');
	convertResult = WideCharToMultiByte(CP_UTF8, 0, str.data(), (int)str.size(), &r[0], (int)r.size(), nullptr, nullptr);
	if (convertResult <= 0) {
		SPDLOG_LOGGER_ERROR(logger, "UTF16ToUTF8 失败");
		assert(false);
		return {};
	}

	return std::string(r.begin(), r.begin() + convertResult);
}

int Utils::Measure(std::function<void()> func) {
    using namespace std::chrono;

    auto t = steady_clock::now();
    func();
    auto dura = duration_cast<milliseconds>(steady_clock::now() - t);

    return int(dura.count());
}

bool Utils::ReadFile(const wchar_t* fileName, std::vector<BYTE>& result) {
	FILE* hFile;
	if (_wfopen_s(&hFile, fileName, L"rb") || !hFile) {
		SPDLOG_LOGGER_ERROR(logger, fmt::format("打开文件{}失败", UTF16ToUTF8(fileName)));
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
		SPDLOG_LOGGER_ERROR(logger, fmt::format("打开文件{}失败", UTF16ToUTF8(fileName)));
		return false;
	}

	// 获取文件长度
	int fd = _fileno(hFile);
	long size = _filelength(fd);

	result.clear();
	result.resize(static_cast<size_t>(size) + 1, 0);

	size_t readed = fread(result.data(), 1, size, hFile);
	result.resize(readed + 1);

	fclose(hFile);
	return true;
}

bool Utils::CompilePixelShader(const char* hlsl, size_t hlslLen, ID3DBlob** blob) {
    ComPtr<ID3DBlob> errorMsgs = nullptr;

    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
    HRESULT hr = D3DCompile(hlsl, hlslLen, nullptr, nullptr, nullptr,
        "PS", "ps_5_0", flags, 0, blob, &errorMsgs);
    if (FAILED(hr)) {
        if (errorMsgs) {
            SPDLOG_LOGGER_ERROR(logger, fmt::sprintf(
                "编译像素着色器失败：%s\n\tHRESULT：0x%X", (const char*)errorMsgs->GetBufferPointer(), hr));
        }
        return false;
    }

    return true;
}
