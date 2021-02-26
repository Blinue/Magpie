#pragma once
#include "pch.h"
#include "Shlwapi.h"
#include <utility>

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

    static D2D1_POINT_2F MapPoint(D2D1_POINT_2F p, const D2D1_RECT_F& src, const D2D1_RECT_F& dest) {
        auto srcSize = GetSize(src);
        auto destSize = GetSize(dest);
        float factor = destSize.width / srcSize.width;

        return {
            (p.x - src.left) * factor + dest.left,
            (p.y - src.top) * factor + dest.top
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

    static void SaveD2DImage(
        ComPtr<ID2D1Device> d2dDevice,
        ComPtr<ID2D1Image> img,
        const std::wstring_view& fileName
    ) {
        ComPtr<IWICImagingFactory2> wicImgFactory;

        Debug::ThrowIfFailed(
            CoCreateInstance(
                CLSID_WICImagingFactory,
                nullptr,
                CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(&wicImgFactory)
            ),
            L""
        );

        ComPtr<IStream> stream;
        Debug::ThrowIfFailed(
            SHCreateStreamOnFileEx(fileName.data(), STGM_WRITE | STGM_CREATE, 0, TRUE, nullptr, &stream),
            L""
        );

        ComPtr<IWICBitmapEncoder> bmpEncoder;
        Debug::ThrowIfFailed(
            wicImgFactory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &bmpEncoder),
            L""
        );
        Debug::ThrowIfFailed(
            bmpEncoder->Initialize(stream.Get(), WICBitmapEncoderNoCache),
            L""
        );

        ComPtr<IWICBitmapFrameEncode> frameEncoder;
        Debug::ThrowIfFailed(
            bmpEncoder->CreateNewFrame(&frameEncoder, nullptr),
            L""
        );
        Debug::ThrowIfFailed(
            frameEncoder->Initialize(nullptr),
            L""
        );

        ComPtr<IWICImageEncoder> d2dImgEncoder;
        Debug::ThrowIfFailed(
            wicImgFactory->CreateImageEncoder(d2dDevice.Get(), &d2dImgEncoder),
            L""
        );
        d2dImgEncoder->WriteFrame(img.Get(), frameEncoder.Get(), nullptr);
        
        Debug::ThrowIfFailed(frameEncoder->Commit(), L"");
        Debug::ThrowIfFailed(bmpEncoder->Commit(), L"");
        stream->Commit(STGC_DEFAULT);
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
