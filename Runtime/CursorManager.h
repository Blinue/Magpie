#pragma once
#include "pch.h"
#include "Utils.h"
#include "Renderable.h"
#include "Env.h"


// 处理光标的渲染
class CursorManager: public Renderable {
public:
    CursorManager(const RECT& destRect, bool debugMode = false) : _destRect(destRect), _debugMode(debugMode) {
        _cursorSize.cx = GetSystemMetrics(SM_CXCURSOR);
        _cursorSize.cy = GetSystemMetrics(SM_CYCURSOR);

        HCURSOR hCursorArrow = LoadCursor(NULL, IDC_ARROW);
        HCURSOR hCursorHand = LoadCursor(NULL, IDC_HAND);
        HCURSOR hCursorAppStarting = LoadCursor(NULL, IDC_APPSTARTING);

        // 保存替换之前的 arrow 光标图像
        // SetSystemCursor 不会改变系统光标的句柄
        _cursorMap.emplace(hCursorArrow, _CursorToD2DBitmap(hCursorArrow));
        _cursorMap.emplace(hCursorHand, _CursorToD2DBitmap(hCursorHand));
        _cursorMap.emplace(hCursorAppStarting, _CursorToD2DBitmap(hCursorAppStarting));

        if (Env::$instance->IsNoDisturb()) {
            return;
        }

        Debug::ThrowIfWin32Failed(
            SetSystemCursor(_CreateTransparentCursor(hCursorArrow), OCR_NORMAL),
            L"设置 OCR_NORMAL 失败"
        );
        Debug::ThrowIfWin32Failed(
            SetSystemCursor(_CreateTransparentCursor(hCursorHand), OCR_HAND),
            L"设置 OCR_HAND 失败"
        );
        Debug::ThrowIfWin32Failed(
            SetSystemCursor(_CreateTransparentCursor(hCursorAppStarting), OCR_APPSTARTING),
            L"设置 OCR_APPSTARTING 失败"
        );

        // 限制鼠标在窗口内
        Debug::ThrowIfWin32Failed(ClipCursor(&Env::$instance->GetSrcClient()), L"ClipCursor 失败");

        // 设置鼠标移动速度
        Debug::ThrowIfWin32Failed(
            SystemParametersInfo(SPI_GETMOUSESPEED, 0, &_cursorSpeed, 0),
            L"获取鼠标速度失败"
        );

        const RECT& srcRect = Env::$instance->GetSrcClient();
        _scale = float(_destRect.right - _destRect.left) / (srcRect.right - srcRect.left);

        long newSpeed = std::clamp(lroundf(_cursorSpeed / _scale), 1L, 20L);
        Debug::ThrowIfWin32Failed(
            SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID)(intptr_t)newSpeed, 0),
            L"设置鼠标速度失败"
        );
    }

	CursorManager(const CursorManager&) = delete;
	CursorManager(CursorManager&&) = delete;

    ~CursorManager() {
        if (Env::$instance->IsNoDisturb()) {
            return;
        }

        ClipCursor(nullptr);

        SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID)(intptr_t)_cursorSpeed, 0);

        // 还原系统光标
        SystemParametersInfo(SPI_SETCURSORS, 0, NULL, 0);
    }

    void Render() override {
        CURSORINFO ci{};
        ci.cbSize = sizeof(ci);
        Debug::ThrowIfWin32Failed(
            GetCursorInfo(&ci),
            L"GetCursorInfo 失败"
        );

        if (_debugMode) {
            _RenderDebugLayer(ci.hCursor);
        }

        if (ci.hCursor == NULL) {
            return;
        }

        _DrawCursor(ci.hCursor, ci.ptScreenPos);
    }


    std::pair<bool, LRESULT> WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
        if (message == _WM_NEWCURSOR32) {
            // 来自 CursorHook 的消息
            // HCURSOR 似乎是共享资源，尽管来自别的进程但可以直接使用
            // 
            // 如果消息来自 32 位进程，本程序为 64 位，必须转换为补符号位扩展，这是为了和 SetCursor 的处理方法一致
            // SendMessage 为补 0 扩展，SetCursor 为补符号位扩展
            _AddHookCursor((HCURSOR)(INT_PTR)(INT32)wParam, (HCURSOR)(INT_PTR)(INT32)lParam);
            return { true, 0 };
        } else if (message == _WM_NEWCURSOR64) {
            // 如果消息来自 64 位进程，本程序为 32 位，HCURSOR 会被截断
            // Q: 如果被截断是否能正常工作？
            _AddHookCursor((HCURSOR)wParam, (HCURSOR)lParam);
            return { true, 0 };
        }

        return { false, 0 };
    }
private:
    void _AddHookCursor(HCURSOR hTptCursor, HCURSOR hCursor) {
        if (hTptCursor == NULL || hCursor == NULL) {
            return;
        }

        _cursorMap[hTptCursor] = _CursorToD2DBitmap(hCursor);
    }

    void _RenderDebugLayer(HCURSOR hCursorCur) {
        D2D1_POINT_2F leftTop{ 0, 50 };

        D2D1_RECT_F cursorRect{
            leftTop.x, leftTop.y, leftTop.x + _cursorSize.cx, leftTop.y + _cursorSize.cy
        };

        ID2D1DeviceContext* d2dDC = Env::$instance->GetD2DDC();
        ComPtr<ID2D1SolidColorBrush> brush;
        d2dDC->CreateSolidColorBrush({ 0,0,0,1 }, &brush);

        for (const auto& a : _cursorMap) {
            d2dDC->FillRectangle(cursorRect, brush.Get());

            if (a.first != hCursorCur) {
                d2dDC->DrawBitmap(a.second.Get(), &cursorRect);
            }

            cursorRect.left += _cursorSize.cx;
            cursorRect.right += _cursorSize.cy;
        }
    }

    void _DrawCursor(HCURSOR hCursor, POINT ptScreenPos) {
        assert(hCursor != NULL);

        ID2D1DeviceContext* d2dDC = Env::$instance->GetD2DDC();

        auto it = _cursorMap.find(hCursor);
        ID2D1Bitmap* bmpCursor = nullptr;

        if (it != _cursorMap.end()) {
            bmpCursor = it->second.Get();
        } else {
            try {
                // 未在映射中找到，创建新映射
                ComPtr<ID2D1Bitmap> newBmpCursor = _CursorToD2DBitmap(hCursor);

                // 添加进映射
                _cursorMap[hCursor] = newBmpCursor;

                bmpCursor = newBmpCursor.Get();
            } catch (...) {
                // 如果出错，只绘制普通光标
                Debug::WriteLine(L"_CursorToD2DBitmap 出错");

                hCursor = LoadCursor(NULL, IDC_ARROW);
                bmpCursor = _cursorMap[hCursor].Get();
            }
        }

        // 映射坐标
        // 鼠标坐标为整数，否则会出现模糊
        const RECT& srcClient = Env::$instance->GetSrcClient();
        D2D1_POINT_2F targetScreenPos{
            roundf((ptScreenPos.x - srcClient.left) * _scale + _destRect.left),
            roundf((ptScreenPos.y - srcClient.top) * _scale + _destRect.top)
        };

        auto [xHotSpot, yHotSpot] = _GetCursorHotSpot(hCursor);

        D2D1_RECT_F rect{};
        Debug::ThrowIfComFailed(
            d2dDC->GetImageLocalBounds(bmpCursor, &rect),
            L"GetImageLocalBounds失败"
        );
        D2D1_RECT_F cursorRect = {
           targetScreenPos.x - xHotSpot,
           targetScreenPos.y - yHotSpot,
           targetScreenPos.x + rect.right - rect.left - xHotSpot,
           targetScreenPos.y + rect.bottom - rect.top - yHotSpot
        };

        d2dDC->DrawBitmap(bmpCursor, &cursorRect);
    }

    HCURSOR _CreateTransparentCursor(HCURSOR hCursorHotSpot) {
        int len = _cursorSize.cx * _cursorSize.cy;
        BYTE* andPlane = new BYTE[len];
        memset(andPlane, 0xff, len);
        BYTE* xorPlane = new BYTE[len]{};

        auto hotSpot = _GetCursorHotSpot(hCursorHotSpot);

        HCURSOR result = CreateCursor(
            Env::$instance->GetHInstance(),
            hotSpot.first, hotSpot.second,
            _cursorSize.cx, _cursorSize.cy,
            andPlane, xorPlane
        );
        Debug::ThrowIfWin32Failed(result, L"创建透明鼠标失败");

        delete[] andPlane;
        delete[] xorPlane;
        return result;
    }

    std::pair<int, int> _GetCursorHotSpot(HCURSOR hCursor) {
        if (hCursor == NULL) {
            return {};
        }

        ICONINFO ii{};
        Debug::ThrowIfWin32Failed(
            GetIconInfo(hCursor, &ii),
            L"GetIconInfo 失败"
        );
        
        DeleteBitmap(ii.hbmColor);
        DeleteBitmap(ii.hbmMask);

        return {
            min((int)ii.xHotspot, (int)_cursorSize.cx),
            min((int)ii.yHotspot, (int)_cursorSize.cy)
        };
    }

    ComPtr<ID2D1Bitmap> _CursorToD2DBitmap(HCURSOR hCursor) {
        assert(hCursor != NULL);

        IWICImagingFactory2* wicImgFactory = Env::$instance->GetWICImageFactory();

        ComPtr<IWICBitmap> wicCursor = nullptr;
        ComPtr<IWICFormatConverter> wicFormatConverter = nullptr;
        ComPtr<ID2D1Bitmap> d2dBmpCursor = nullptr;

        Debug::ThrowIfComFailed(
            wicImgFactory->CreateBitmapFromHICON(hCursor, &wicCursor),
            L"创建鼠标图像位图失败"
        );
        Debug::ThrowIfComFailed(
            wicImgFactory->CreateFormatConverter(&wicFormatConverter),
            L"CreateFormatConverter 失败"
        );
        Debug::ThrowIfComFailed(
            wicFormatConverter->Initialize(
                wicCursor.Get(),
                GUID_WICPixelFormat32bppPBGRA,
                WICBitmapDitherTypeNone,
                NULL,
                0.f,
                WICBitmapPaletteTypeMedianCut
            ),
            L"IWICFormatConverter 初始化失败"
        );
        Debug::ThrowIfComFailed(
            Env::$instance->GetD2DDC()->CreateBitmapFromWicBitmap(wicFormatConverter.Get(), &d2dBmpCursor),
            L"CreateBitmapFromWicBitmap 失败"
        );

        return d2dBmpCursor;
    }


    std::map<HCURSOR, ComPtr<ID2D1Bitmap>> _cursorMap;

    SIZE _cursorSize{};

    RECT _destRect;
    float _scale;

    INT _cursorSpeed = 0;

    bool _debugMode = false;

    static UINT _WM_NEWCURSOR32;
    static UINT _WM_NEWCURSOR64;
};
