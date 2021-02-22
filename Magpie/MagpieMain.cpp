#include "pch.h"
// WindowsProject3.cpp : 定义应用程序的入口点。
//
// dllmain.cpp : 定义 DLL 应用程序的入口点。

#include "Utils.h"
#include "KeyBoardHook.h"
#include "MagWindow.h"
using namespace D2D1;


struct {
    HCURSOR normal;
    HCURSOR hand;
} systemCursors{};

struct {
    HCURSOR normal;
    HCURSOR hand;
} tptCursors{};

std::unique_ptr<MagWindow> magWnd = nullptr;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow) {

    SetProcessDPIAware();

    KeyBoardHook::hook({ VK_LMENU, VK_RMENU, VK_F11 });
    KeyBoardHook::setKeyDownCallback([=](int key) {
        std::vector<int> keys = KeyBoardHook::getPressedKeys();
        if ((std::find(keys.begin(), keys.end(), VK_LMENU) != keys.end() || std::find(keys.begin(), keys.end(), VK_RMENU) != keys.end())
            && std::find(keys.begin(), keys.end(), VK_F11) != keys.end()
        ) {
            if (magWnd == nullptr) {
                magWnd.reset(new MagWindow(hInstance, GetForegroundWindow(), 60, LR"(
[
  {
    "effect": "scale",
    "type": "anime4K"
  },
  {
    "effect": "scale",
    "type": "jinc2",
    "scale": [0,0]
  },
  {
    "effect": "sharpen",
    "type": "adaptive",
    "strength": 0.6
  }
])"));
            } else {
                magWnd = nullptr;
            }
        }
    });

    /*tptCursors.normal = CreateTransparentCursor();
    if (tptCursors.normal == NULL) {
        return 1;
    }
    tptCursors.hand = CopyCursor(tptCursors.normal);*/
    
    MSG msg;
    // 主消息循环:
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    /*if (tptCursors.normal) {
        DestroyCursor(tptCursors.normal);
    }
    if (systemCursors.normal) {
        DestroyCursor(systemCursors.normal);
    }
    if (tptCursors.hand) {
        DestroyCursor(tptCursors.hand);
    }
    if (systemCursors.hand) {
        DestroyCursor(systemCursors.hand);
    }*/
    
    return (int)msg.wParam;
}

/*
ComPtr<ID2D1Bitmap> d2dBmpNormalCursor = nullptr;
ComPtr<ID2D1Bitmap> d2dBmpHandCursor = nullptr;

ComPtr<ID2D1Bitmap> ToD2DBitmap(HCURSOR hCursor) {
    ComPtr<IWICBitmap> wicCursor = nullptr;
    ComPtr<IWICFormatConverter> wicFormatConverter = nullptr;
    ComPtr<ID2D1Bitmap> d2dBmpCursor = nullptr;

    Debug::ThrowIfFailed(
        wicImgFactory->CreateBitmapFromHICON(hCursor, &wicCursor)
    );
    Debug::ThrowIfFailed(
        wicImgFactory->CreateFormatConverter(&wicFormatConverter)
    );
    Debug::ThrowIfFailed(
        wicFormatConverter->Initialize(wicCursor.Get(), GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.f, WICBitmapPaletteTypeMedianCut)
    );
    Debug::ThrowIfFailed(
        _d2dDC->CreateBitmapFromWicBitmap(wicFormatConverter.Get(), &d2dBmpCursor)
    );

    return d2dBmpCursor;
}

bool InitD2D() {
    d2dBmpNormalCursor = ToD2DBitmap(systemCursors.normal);
    d2dBmpHandCursor = ToD2DBitmap(systemCursors.hand);

    return true;
}


void DrawCursor(ComPtr<ID2D1DeviceContext> _d2dDC) {
    CURSORINFO ci{};
    ci.cbSize = sizeof(ci);
    GetCursorInfo(&ci);

    if (ci.hCursor == NULL) {
        return;
    }

    POINT targetScreenPos = Utils::MapPoint(ci.ptScreenPos, srcClient, destClient);

    int cursorWidth = GetSystemMetrics(SM_CXCURSOR);
    int cursorHeight = GetSystemMetrics(SM_CYCURSOR);

    D2D1_RECT_F cursorRect {
        float(targetScreenPos.x),
        float(targetScreenPos.y),
        float(targetScreenPos.x + cursorWidth),
        float(targetScreenPos.y + cursorHeight)
    };

    
    if (ci.hCursor == LoadCursor(NULL, IDC_ARROW)) {
        _d2dDC->DrawBitmap(d2dBmpNormalCursor.Get(), &cursorRect);
    } else if(ci.hCursor == LoadCursor(NULL, IDC_HAND)) {
        _d2dDC->DrawBitmap(d2dBmpHandCursor.Get(), &cursorRect);
    } else {
        ComPtr<ID2D1Bitmap> c = ToD2DBitmap(CopyCursor(ci.hCursor));
        _d2dDC->DrawBitmap(c.Get(), &cursorRect);
    }
    
}


HCURSOR CreateTransparentCursor() {
    int cursorWidth = GetSystemMetrics(SM_CXCURSOR);
    int cursorHeight = GetSystemMetrics(SM_CYCURSOR);

    int len = cursorWidth * cursorHeight;
    BYTE* andPlane = new BYTE[len];
    memset(andPlane, 0xff, len);
    BYTE* xorPlane = new BYTE[len]{};
    
    HCURSOR _result = CreateCursor(hDllModule, 0, 0, cursorWidth, cursorHeight, andPlane, xorPlane);

    delete[] andPlane;
    delete[] xorPlane;
    return _result;
}
*/