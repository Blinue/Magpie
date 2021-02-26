#pragma once
#include "pch.h"


class CursorManager {
public:
	CursorManager(
        HINSTANCE hInstance,
        const ComPtr<IWICImagingFactory2>& wicImgFactory,
        const ComPtr<ID2D1DeviceContext>& d2dDC,
        const RECT& srcClient,
        bool noDisturb = false
    ): _hInstance(hInstance), _wicImgFactory(wicImgFactory), _d2dDC(d2dDC), _srcClient(srcClient) {
        _cursorSize.cx = GetSystemMetrics(SM_CXCURSOR);
        _cursorSize.cy = GetSystemMetrics(SM_CYCURSOR);

        _systemCursors.hand = CopyCursor(LoadCursor(NULL, IDC_HAND));
        _systemCursors.normal = CopyCursor(LoadCursor(NULL, IDC_ARROW));

        _d2dBmpNormalCursor = _CursorToD2DBitmap(_systemCursors.normal);
        _d2dBmpHandCursor = _CursorToD2DBitmap(_systemCursors.hand);
        
        if (!noDisturb) {
            HCURSOR tptCursor = _CreateTransparentCursor();
            Debug::ThrowIfFalse(tptCursor, L"");

            SetSystemCursor(CopyCursor(tptCursor), OCR_HAND);
            SetSystemCursor(CopyCursor(tptCursor), OCR_NORMAL);
            ClipCursor(&srcClient);

            DestroyCursor(tptCursor);
        }
	}

	CursorManager(const CursorManager&) = delete;
	CursorManager(CursorManager&&) = delete;

    ~CursorManager() {
        ClipCursor(nullptr);

        SetSystemCursor(_systemCursors.normal, OCR_NORMAL);
        SetSystemCursor(_systemCursors.hand, OCR_HAND);
        // _systemCursors 中的资源已释放
    }

	void DrawCursor(const RECT& destClient) {
        CURSORINFO ci{};
        ci.cbSize = sizeof(ci);
        GetCursorInfo(&ci);

        if (ci.hCursor == NULL) {
            return;
        }

        POINT targetScreenPos = Utils::MapPoint(ci.ptScreenPos, _srcClient, destClient);

        D2D1_RECT_F cursorRect{
            float(targetScreenPos.x),
            float(targetScreenPos.y),
            float(targetScreenPos.x + _cursorSize.cx),
            float(targetScreenPos.y + _cursorSize.cy)
        };

        if (ci.hCursor == LoadCursor(NULL, IDC_ARROW)) {
            _d2dDC->DrawBitmap(_d2dBmpNormalCursor.Get(), &cursorRect);
        } else if (ci.hCursor == LoadCursor(NULL, IDC_HAND)) {
            _d2dDC->DrawBitmap(_d2dBmpHandCursor.Get(), &cursorRect);
        } else {
            ComPtr<ID2D1Bitmap> c = _CursorToD2DBitmap(CopyCursor(ci.hCursor));
            _d2dDC->DrawBitmap(c.Get(), &cursorRect);
            //_d2dDC->DrawBitmap(_d2dBmpNormalCursor.Get(), &cursorRect);
        }
	}

private:
    HCURSOR _CreateTransparentCursor() {
        int len = _cursorSize.cx * _cursorSize.cy;
        BYTE* andPlane = new BYTE[len];
        memset(andPlane, 0xff, len);
        BYTE* xorPlane = new BYTE[len]{};

        HCURSOR _result = CreateCursor(_hInstance, 0, 0, _cursorSize.cx, _cursorSize.cy, andPlane, xorPlane);

        delete[] andPlane;
        delete[] xorPlane;
        return _result;
    }

    ComPtr<ID2D1Bitmap> _CursorToD2DBitmap(HCURSOR hCursor) {
        ComPtr<IWICBitmap> wicCursor = nullptr;
        ComPtr<IWICFormatConverter> wicFormatConverter = nullptr;
        ComPtr<ID2D1Bitmap> d2dBmpCursor = nullptr;

        Debug::ThrowIfFailed(
            _wicImgFactory->CreateBitmapFromHICON(hCursor, &wicCursor),
            L""
        );
        Debug::ThrowIfFailed(
            _wicImgFactory->CreateFormatConverter(&wicFormatConverter),
            L""
        );
        Debug::ThrowIfFailed(
            wicFormatConverter->Initialize(
                wicCursor.Get(),
                GUID_WICPixelFormat32bppPBGRA,
                WICBitmapDitherTypeNone,
                NULL,
                0.f,
                WICBitmapPaletteTypeMedianCut
            ),
            L""
        );
        Debug::ThrowIfFailed(
            _d2dDC->CreateBitmapFromWicBitmap(wicFormatConverter.Get(), &d2dBmpCursor),
            L""
        );

        return d2dBmpCursor;
    }

    HINSTANCE _hInstance;
    ComPtr<IWICImagingFactory2> _wicImgFactory;
    ComPtr<ID2D1DeviceContext> _d2dDC;

    ComPtr<ID2D1Bitmap> _d2dBmpNormalCursor = nullptr;
    ComPtr<ID2D1Bitmap> _d2dBmpHandCursor = nullptr;

    SIZE _cursorSize{};

    RECT _srcClient;

    struct {
        HCURSOR normal;
        HCURSOR hand;
    } _systemCursors{};
};
