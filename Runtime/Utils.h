#pragma once
#include "pch.h"
#include <utility>
#include <wrl.h>


extern std::shared_ptr<spdlog::logger> logger;

class Utils {
public:
	static UINT GetWindowShowCmd(HWND hwnd) {
		assert(hwnd != NULL);

		WINDOWPLACEMENT wp{};
		wp.length = sizeof(wp);
		GetWindowPlacement(hwnd, &wp);

		return wp.showCmd;
	}

	static bool GetClientScreenRect(HWND hWnd, RECT& rect) {
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

	static RECT GetScreenRect(HWND hWnd) {
		HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);

		MONITORINFO mi{};
		mi.cbSize = sizeof(mi);
		GetMonitorInfo(hMonitor, &mi);
		return mi.rcMonitor;
	}

	static SIZE GetSize(const RECT& rect) {
		return { rect.right - rect.left, rect.bottom - rect.top };
	}

	static D2D1_SIZE_F GetSize(const D2D1_RECT_F& rect) {
		return { rect.right - rect.left,rect.bottom - rect.top };
	}

	static BOOL Str2GUID(const std::wstring_view &szGUID, GUID& guid) {
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


	static std::string GUID2Str(GUID guid) {
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

	static std::wstring UTF8ToUTF16(std::string_view str) {
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

	static std::string UTF16ToUTF8(std::wstring_view str) {
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

	static int Measure(std::function<void()> func) {
		using namespace std::chrono;

		auto t = steady_clock::now();
		func();
		auto dura = duration_cast<milliseconds>(steady_clock::now() - t);

		return int(dura.count());
	}
};

namespace std {
	// std::hash 的 GUID 特化
	template<>
	struct hash<GUID> {
		size_t operator()(const GUID& value) const {
			size_t result = hash<unsigned long>()(value.Data1);
			result ^= hash<unsigned short>()(value.Data2) << 1;
			result ^= hash<unsigned short>()(value.Data3) << 2;
			
			for (int i = 0; i < 8; ++i) {
				result ^= hash<unsigned short>()(value.Data4[i]) << i;
			}

			return result;
		}
	};
}
