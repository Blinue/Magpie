#pragma once
#include "pch.h"
#include <vector>
#include <map>
#include "Utils.h"
#include "EffectRenderer.h"


using namespace std;

class MagWindow {
public:
	MagWindow(HINSTANCE hInstance, HWND hwndSrc, UINT frameRate, const std::wstring_view& effectsJson) 
        : _hInst(hInstance), _hwndSrc(hwndSrc), _frameRate(frameRate)
    {
        assert(_frameRate >= 1 && _frameRate <= 200);

        Debug::ThrowIfFalse(
            IsWindow(_hwndSrc) && IsWindowVisible(_hwndSrc),
            L"hwndSrc 不合法"
        );

        Debug::ThrowIfFalse(
            Utils::GetClientScreenRect(_hwndSrc, _srcClient),
            L"无法获取源窗口客户区尺寸"
        );

        _RegisterHostWndClass();
        _CreateHostWnd();
        _InitWICImgFactory();

        // 初始化 EffectRenderer
        _effectRenderer.reset(new EffectRenderer(_hwndHost, effectsJson, Utils::GetSize(_srcClient)));

        /*ClipCursor(&srcClient);

        systemCursors.hand = CopyCursor(LoadCursor(NULL, IDC_HAND));
        systemCursors.normal = CopyCursor(LoadCursor(NULL, IDC_ARROW));
        SetSystemCursor(CopyCursor(tptCursors.hand), OCR_HAND);
        SetSystemCursor(CopyCursor(tptCursors.normal), OCR_NORMAL);*/
        ShowWindow(_hwndHost, SW_NORMAL);
        SetTimer(_hwndHost, 1, 1000 / _frameRate, nullptr);
	}
    
    ~MagWindow() {
        DestroyWindow(_hwndHost);
    }

    static LRESULT CALLBACK HostWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
        switch (message) {
        case WM_CREATE:
        {
            MagInitialize();
            break;
        }
        case WM_DESTROY:
        {
            _instMap.erase(hWnd);
            MagUninitialize();
            //PostQuitMessage(0);

            /*hwndHost = NULL;
            hwndSrc = NULL;
            hwndMag = NULL;

            ClipCursor(NULL);

            SetSystemCursor(systemCursors.hand, OCR_HAND);
            SetSystemCursor(systemCursors.normal, OCR_NORMAL);
            systemCursors.hand = NULL;
            systemCursors.normal = NULL;*/
            break;
        }
        case WM_TIMER:
        {
            MagWindow* that = GetInstance(hWnd);
            if (!that) {
                // 窗口已销毁
                KillTimer(hWnd, wParam);
                break;
            }

            // 渲染一帧
            MagSetWindowSource(that->_hwndMag, that->_srcClient);
            break;
        }
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        return 0;
    }


private:
    // 注册全屏窗口类
    void _RegisterHostWndClass() {
        WNDCLASSEX wcex = {};
        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.lpfnWndProc = HostWndProc;
        wcex.hInstance = _hInst;
        wcex.lpszClassName = _HOST_WINDOW_CLASS_NAME;

        // 忽略重复注册造成的错误
        RegisterClassEx(&wcex);
    }

    void _CreateHostWnd() {
        // 创建全屏窗口
        SIZE screenSize = Utils::GetScreenSize(_hwndSrc);
        _hwndHost = CreateWindowEx(
            WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_LAYERED | WS_EX_TRANSPARENT,
            _HOST_WINDOW_CLASS_NAME, NULL, WS_CLIPCHILDREN | WS_POPUP | WS_VISIBLE,
            0, 0, screenSize.cx, screenSize.cy,
            NULL, NULL, _hInst, NULL);
        Debug::ThrowIfFalse(_hwndHost, L"创建全屏窗口失败");

        _instMap[_hwndHost] = this;

        SetLayeredWindowAttributes(_hwndHost, 0, 255, LWA_ALPHA);

        // 创建不可见的放大镜控件
        // 大小为目标窗口客户区
        SIZE srcClientSize = Utils::GetSize(_srcClient);

        _hwndMag = CreateWindow(WC_MAGNIFIER, L"MagnifierWindow",
            WS_CHILD,
            0, 0, srcClientSize.cx, srcClientSize.cy,
            _hwndHost, NULL, _hInst, NULL);
        Debug::ThrowIfFalse(_hwndMag, L"创建放大镜控件失败");

        Debug::ThrowIfFalse(
            MagSetImageScalingCallback(_hwndMag, &_ImageScalingCallback),
            L"设置放大镜回调失败"
        );

        
        //ShowWindow(_hwndHost, SW_SHOW);
    }

    static BOOL CALLBACK _ImageScalingCallback(
        HWND hWnd,
        void* srcdata,
        MAGIMAGEHEADER srcheader,
        void* destdata,
        MAGIMAGEHEADER destheader,
        RECT unclipped,
        RECT clipped,
        HRGN dirty
    ) {
        MagWindow* that = _instMap.begin()->second;//GetInstance(GetParent(hWnd));
        
        Debug::ThrowIfFalse(
            srcheader.width * srcheader.height * 4 == srcheader.cbSize,
            L"srcdata 不是BGRA格式"
        );

        ComPtr<IWICBitmap> wicBmpSource = nullptr;
        Debug::ThrowIfFailed(
            that->_wicImgFactory->CreateBitmapFromMemory(
                srcheader.width,
                srcheader.height,
                GUID_WICPixelFormat32bppPBGRA,
                srcheader.stride,
                (UINT)srcheader.cbSize,
                (BYTE*)srcdata,
                &wicBmpSource
            ),
            L"从内存创建 WICBitmap 失败"
        );

        that->_effectRenderer->Render(wicBmpSource);

        //DrawCursor(_d2dDC);

        return TRUE;
    }


    void _InitWICImgFactory() {
        Debug::ThrowIfFailed(
            CoInitialize(NULL), 
            L"初始化 COM 出错"
        );

        Debug::ThrowIfFailed(
            CoCreateInstance(
                CLSID_WICImagingFactory,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(&_wicImgFactory)
            ),
            L"创建 WICImagingFactory 失败"
        );
    }

    // 查找 MagWindow 实例
    static MagWindow* GetInstance(HWND hWnd) {
        return _instMap[hWnd];
    }

    // 全屏窗口类名
    const wchar_t* _HOST_WINDOW_CLASS_NAME = L"Window_Magpie_967EB565-6F73-4E94-AE53-00CC42592A22";

    HINSTANCE _hInst;
    HWND _hwndHost = NULL;
    HWND _hwndMag = NULL;
    HWND _hwndSrc;
    RECT _srcClient{};
    UINT _frameRate;
    ComPtr<IWICImagingFactory2> _wicImgFactory = nullptr;
    std::unique_ptr<EffectRenderer> _effectRenderer = nullptr;

    // 存储所有实例，可通过窗口句柄查找
    static std::map<HWND, MagWindow*> _instMap;
};
