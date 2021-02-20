#pragma once
#include "pch.h"

class Utils {
public:
    static bool GetClientScreenRect(HWND hWnd, RECT& rect) {
        RECT r;
        if (!GetClientRect(hWnd, &r)) {
            return false;
        }

        POINT p{};
        if (!ClientToScreen(hWnd, &p)) {
            return false;
        }

        rect.bottom = r.bottom + p.y;
        rect.left = r.left + p.x;
        rect.right = r.right + p.x;
        rect.top = r.top + p.y;

        return true;
    }

    static SIZE GetScreenSize() {
        return { GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
    }

    static SIZE GetScreenSize(HWND hWnd) {
        HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);

        MONITORINFO mi{};
        mi.cbSize = sizeof(mi);
        Debug::ThrowIfFalse(
            GetMonitorInfo(hMonitor, &mi),
            L"获取显示器信息失败"
        );
        return GetSize(mi.rcMonitor);
    }

    static SIZE GetSize(const RECT& rect) {
        return { rect.right - rect.left, rect.bottom - rect.top };
    }

    static D2D1_SIZE_F GetSize(const D2D1_RECT_F& rect) {
        return { rect.right - rect.left,rect.bottom - rect.top };
    }

    static POINT MapPoint(POINT p, const RECT& src, const RECT& dest) {
        SIZE srcSize = GetSize(src);
        SIZE destSize = GetSize(dest);
        double factor = (double)destSize.cx / srcSize.cx;

        return {
            (int)lround((p.x - src.left) * factor) + dest.left,
            (int)lround((p.y - src.top) * factor) + dest.top
        };
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
};