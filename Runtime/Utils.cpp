#include "pch.h"
#include "Utils.h"
#include <io.h>


extern std::shared_ptr<spdlog::logger> logger;

UINT Utils::GetWindowShowCmd(HWND hwnd) {
	assert(hwnd != NULL);

	WINDOWPLACEMENT wp{};
	wp.length = sizeof(wp);
	if (!GetWindowPlacement(hwnd, &wp)) {
		SPDLOG_LOGGER_ERROR(logger,
			fmt::format("GetWindowPlacement 出错\n\tLastErrorCode：{}", GetLastError()));
		assert(false);
	}

	return wp.showCmd;
}

RECT Utils::GetClientScreenRect(HWND hWnd) {
	RECT r;
	if (!GetClientRect(hWnd, &r)) {
		SPDLOG_LOGGER_ERROR(logger,
			fmt::format("GetClientRect 出错\n\tLastErrorCode：{}", GetLastError()));
		assert(false);
		return {};
	}

	POINT p{};
	if (!ClientToScreen(hWnd, &p)) {
		SPDLOG_LOGGER_ERROR(logger,
			fmt::format("ClientToScreen 出错\n\tLastErrorCode：{}", GetLastError()));
		assert(false);
		return {};
	}

	r.bottom += p.y;
	r.left += p.x;
	r.right += p.x;
	r.top += p.y;

	return r;
}

RECT Utils::GetScreenRect(HWND hWnd) {
	HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);

	MONITORINFO mi{};
	mi.cbSize = sizeof(mi);
	if (!GetMonitorInfo(hMonitor, &mi)) {
		SPDLOG_LOGGER_ERROR(logger,
			fmt::format("GetMonitorInfo 出错\n\tLastErrorCode：{}", GetLastError()));
		assert(false);
	}
	return mi.rcMonitor;
}

std::wstring Utils::UTF8ToUTF16(std::string_view str) {
	int convertResult = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), nullptr, 0);
	if (convertResult <= 0) {
		SPDLOG_LOGGER_ERROR(logger, fmt::format("UTF8ToUTF16 失败\n\tLastErrorCode：{}", GetLastError()));
		assert(false);
		return {};
	}

	std::wstring r(convertResult + 10, L'\0');
	convertResult = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), &r[0], (int)r.size());
	if (convertResult <= 0) {
		SPDLOG_LOGGER_ERROR(logger, fmt::format("UTF8ToUTF16 失败\n\tLastErrorCode：{}", GetLastError()));
		assert(false);
		return {};
	}

	return std::wstring(r.begin(), r.begin() + convertResult);
}

std::string Utils::UTF16ToUTF8(std::wstring_view str) {
	int convertResult = WideCharToMultiByte(CP_UTF8, 0, str.data(), (int)str.size(), nullptr, 0, nullptr, nullptr);
	if (convertResult <= 0) {
		SPDLOG_LOGGER_ERROR(logger, fmt::format("UTF16ToUTF8 失败\n\tLastErrorCode：{}", GetLastError()));
		assert(false);
		return {};
	}

	std::string r(convertResult + 10, L'\0');
	convertResult = WideCharToMultiByte(CP_UTF8, 0, str.data(), (int)str.size(), &r[0], (int)r.size(), nullptr, nullptr);
	if (convertResult <= 0) {
		SPDLOG_LOGGER_ERROR(logger, fmt::format("UTF16ToUTF8 失败\n\tLastErrorCode：{}", GetLastError()));
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
        "main", "ps_5_0", flags, 0, blob, &errorMsgs);
    if (FAILED(hr)) {
        if (errorMsgs) {
            SPDLOG_LOGGER_ERROR(logger, fmt::sprintf(
                "编译像素着色器失败：%s\n\tHRESULT：0x%X", (const char*)errorMsgs->GetBufferPointer(), hr));
        }
        return false;
    }

    return true;
}
