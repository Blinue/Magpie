#pragma once
#include "pch.h"
#include "Utils.h"
#include "EffectRenderer.h"


class MagWindow {
public:
	MagWindow(
        HINSTANCE hInstance, 
        HWND hwndSrc, 
        UINT frameRate,
        const std::wstring_view& effectsJson,
        bool noDisturb = false
    ) : _hInst(hInstance), _hwndSrc(hwndSrc), _frameRate(frameRate)
    {
        if (_instance) {
            Debug::ThrowIfFalse(false, L"已存在全屏窗口");
        }

        assert(_frameRate == 0 || (_frameRate >= 30 && _frameRate <= 120));

        Debug::ThrowIfFalse(
            IsWindow(_hwndSrc) && IsWindowVisible(_hwndSrc),
            L"hwndSrc 不合法"
        );

        Debug::ThrowIfFalse(
            Utils::GetClientScreenRect(_hwndSrc, _srcRect),
            L"无法获取源窗口客户区尺寸"
        );

        _RegisterHostWndClass();

        Debug::ThrowIfFalse(
            MagInitialize(),
            L"MagInitialize 失败"
        );
        _CreateHostWnd(noDisturb);

        _InitWICImgFactory();

        // 初始化 EffectRenderer
        _effectRenderer.reset(
            new EffectRenderer(
                hInstance,
                _hwndHost,
                effectsJson,
                _srcRect,
                _wicImgFactory,
                noDisturb
            )
        );

        ShowWindow(_hwndHost, SW_NORMAL);

        if (_frameRate > 0) {
            SetTimer(_hwndHost, 1, 1000 / _frameRate, nullptr);
        } else {
            PostMessage(_hwndHost, WM_MAXFRAMERATE, 0, 0);
        }

        // 初始化完成后设置 _instance
        _instance = this;
	}

    // 不可复制，不可移动
    MagWindow(const MagWindow&) = delete;
    MagWindow(MagWindow&&) = delete;
    
    ~MagWindow() {
        MagUninitialize();

        DestroyWindow(_hwndHost);
        _instance = nullptr;

        UnregisterClass(_HOST_WINDOW_CLASS_NAME, _hInst);
    }

    static LRESULT CALLBACK HostWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
        switch (message) {
        case WM_TIMER:
        {
            if (!_instance) {
                // timer此时已被销毁
                break;
            }

            // 渲染一帧
            if (!MagSetWindowSource(_instance->_hwndMag, _instance->_srcRect)) {
                Debug::writeLine(L"MagSetWindowSource 失败");
            }
            break;
        }
        case WM_MAXFRAMERATE:
        {
            if (!_instance) {
                // 窗口已销毁，直接退出
                break;
            }

            // 渲染一帧
            if (!MagSetWindowSource(_instance->_hwndMag, _instance->_srcRect)) {
                Debug::writeLine(L"MagSetWindowSource 失败");
            }

            // 立即渲染下一帧
            if (!PostMessage(hWnd, WM_MAXFRAMERATE, 0, 0)) {
                Debug::writeLine(L"PostMessage 失败");
            }
            break;
        }
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        return 0;
    }


private:
    // 注册全屏窗口类
    void _RegisterHostWndClass() const {
        WNDCLASSEX wcex = {};
        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.lpfnWndProc = HostWndProc;
        wcex.hInstance = _hInst;
        wcex.lpszClassName = _HOST_WINDOW_CLASS_NAME;

        // 忽略重复注册造成的错误
        RegisterClassEx(&wcex);
    }

    void _CreateHostWnd(bool noDisturb) {
        // 创建全屏窗口
        SIZE screenSize = Utils::GetScreenSize(_hwndSrc);
        _hwndHost = CreateWindowEx(
            (noDisturb ? 0 : WS_EX_TOPMOST) | WS_EX_NOACTIVATE | WS_EX_LAYERED | WS_EX_TRANSPARENT,
            _HOST_WINDOW_CLASS_NAME, NULL, WS_CLIPCHILDREN | WS_POPUP | WS_VISIBLE,
            0, 0, screenSize.cx, screenSize.cy,
            NULL, NULL, _hInst, NULL);
        Debug::ThrowIfFalse(_hwndHost, L"创建全屏窗口失败");

        // 设置窗口不透明
        Debug::ThrowIfFalse(
            SetLayeredWindowAttributes(_hwndHost, 0, 255, LWA_ALPHA),
            L"SetLayeredWindowAttributes 失败"
        );

        // 创建不可见的放大镜控件
        // 大小为目标窗口客户区
        SIZE srcClientSize = Utils::GetSize(_srcRect);

        _hwndMag = CreateWindow(WC_MAGNIFIER, L"MagnifierWindow",
            WS_CHILD,
            0, 0, srcClientSize.cx, srcClientSize.cy,
            _hwndHost, NULL, _hInst, NULL);
        Debug::ThrowIfFalse(_hwndMag, L"创建放大镜控件失败");

        Debug::ThrowIfFalse(
            MagSetImageScalingCallback(_hwndMag, &_ImageScalingCallback),
            L"设置放大镜回调失败"
        );
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
        if (!_instance) {
            return FALSE;
        }
        
        if (srcheader.cbSize / srcheader.width / srcheader.height != 4) {
            Debug::writeLine(L"srcdata 不是BGRA格式");
            return FALSE;
        }

        ComPtr<IWICBitmap> wicBmpSource = nullptr;

        try {
            Debug::ThrowIfFailed(
                _instance->_wicImgFactory->CreateBitmapFromMemory(
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

            _instance->_effectRenderer->Render(wicBmpSource);
        } catch (...) {
            Debug::writeLine(L"渲染出错");
            return FALSE;
        }
        
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

    // 全屏窗口类名
    static constexpr const wchar_t* _HOST_WINDOW_CLASS_NAME = L"Window_Magpie_967EB565-6F73-4E94-AE53-00CC42592A22";

    // 指定为最大帧率时使用的渲染消息
    static constexpr const UINT WM_MAXFRAMERATE = WM_USER;

    HINSTANCE _hInst;
    HWND _hwndHost = NULL;
    HWND _hwndMag = NULL;
    HWND _hwndSrc;
    RECT _srcRect{};
    UINT _frameRate;

    ComPtr<IWICImagingFactory2> _wicImgFactory = nullptr;
    std::unique_ptr<EffectRenderer> _effectRenderer = nullptr;

    // 确保只能同时存在一个全屏窗口
    static MagWindow *_instance;
};
