#pragma once
#include "pch.h"
#include "Utils.h"
#include "EffectRenderer.h"
#include "MagCallbackWindowCapturer.h"
#include "GDIWindowCapturer.h"


class MagWindow {
public:
    // 确保只能同时存在一个全屏窗口
    static std::unique_ptr<MagWindow> $instance;

    static void CreateInstance(
        HINSTANCE hInstance,
        HWND hwndSrc,
        UINT frameRate,
        const std::wstring_view& effectsJson,
        bool noDisturb = false
    ) {
        $instance.reset(new MagWindow(
            hInstance,
            hwndSrc,
            frameRate,
            effectsJson,
            noDisturb
        ));
    }

    // 不可复制，不可移动
    MagWindow(const MagWindow&) = delete;
    MagWindow(MagWindow&&) = delete;
    
    ~MagWindow() {
        DestroyWindow(_hwndHost);
        UnregisterClass(_HOST_WINDOW_CLASS_NAME, _hInst);

        CoUninitialize();
    }

    HWND GetSrcWnd() {
        return _hwndSrc;
    }

    HWND GetHostWnd() {
        return _hwndHost;
    }

private:
    MagWindow(
        HINSTANCE hInstance,
        HWND hwndSrc,
        UINT frameRate,
        const std::wstring_view& effectsJson,
        bool noDisturb
    ) : _hInst(hInstance), _hwndSrc(hwndSrc), _frameRate(frameRate) {
        if ($instance) {
            Debug::Assert(false, L"已存在全屏窗口");
        }

        assert(_frameRate == 0 || (_frameRate >= 30 && _frameRate <= 120));

        Debug::ThrowIfComFailed(
            CoInitialize(NULL),
            L"初始化 COM 出错"
        );
        
        Debug::Assert(
            IsWindow(_hwndSrc) && IsWindowVisible(_hwndSrc),
            L"hwndSrc 不合法"
        );

        Utils::GetClientScreenRect(_hwndSrc, _srcRect);

        _RegisterHostWndClass();
        _CreateHostWnd(noDisturb);

        Debug::ThrowIfComFailed(
            CoCreateInstance(
                CLSID_WICImagingFactory,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(&_wicImgFactory)
            ),
            L"创建 WICImagingFactory 失败"
        );

        /*_windowCapturer.reset(new MagCallbackWindowCapturer(
            hInstance,
            _hwndHost,
            _srcRect,
            _wicImgFactory.Get()
        ));*/
        _windowCapturer.reset(new GDIWindowCapturer(
            _hwndSrc,
            _srcRect,
            _wicImgFactory.Get(),
            true
        ));

        // 初始化 EffectRenderer
        _effectRenderer.reset(
            new EffectRenderer(
                hInstance,
                _hwndHost,
                effectsJson,
                _srcRect,
                _wicImgFactory.Get(),
                noDisturb
            )
        );

        // 初始化 CursorManager
        _cursorManager.reset(new CursorManager(
            hInstance, _wicImgFactory.Get(), _effectRenderer->GetD2DDC(),
            D2D1_RECT_F{ (FLOAT)_srcRect.left, (FLOAT)_srcRect.top, (FLOAT)_srcRect.right, (FLOAT)_srcRect.bottom },
            _effectRenderer->GetDestRect(), noDisturb)
        );

        ShowWindow(_hwndHost, SW_NORMAL);

        if (_frameRate > 0) {
            SetTimer(_hwndHost, 1, 1000 / _frameRate, nullptr);
        } else {
            PostMessage(_hwndHost, _WM_MAXFRAMERATE, 0, 0);
        }

        _RegisterHookMsg();
    }

    // 渲染一帧，不抛出异常
    static void _Render() {
        try {
            ComPtr<IWICBitmapSource> frame = $instance->_windowCapturer->GetFrame();
            $instance->_effectRenderer->Render(frame.Get(), *$instance->_cursorManager);
        } catch (const magpie_exception& e) {
            Debug::WriteErrorMessage(L"渲染失败：" + e.what());
        } catch (...) {
            Debug::WriteErrorMessage(L"渲染出现未知错误");
        }
    }

    static LRESULT CALLBACK _HostWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
        switch (message) {
        case WM_TIMER:
        {
            if (!$instance) {
                // timer此时已被销毁
                break;
            }

            /* 已改为接收 shell 消息
            if (GetForegroundWindow() != $instance->_hwndSrc) {
                DestroyMagWindow();
                break;
            }*/

            _Render();

            break;
        }
        case _WM_MAXFRAMERATE:
        {
            if (!$instance) {
                // 窗口已销毁，直接退出
                break;
            }

            /* 已改为接收 shell 消息
            if (GetForegroundWindow() != $instance->_hwndSrc) {
                DestroyMagWindow();
                break;
            }*/

            _Render();

            // 立即渲染下一帧
            if (!PostMessage(hWnd, _WM_MAXFRAMERATE, 0, 0)) {
                Debug::WriteErrorMessage(L"PostMessage 失败");
            }
            break;
        }
        default:
        {
            if ($instance == nullptr) {
                return DefWindowProc(hWnd, message, wParam, lParam);
            }

            if (message == $instance->_WM_SHELLHOOKMESSAGE) {
                // 在桌面环境变化时关闭全屏窗口
                // 文档没提到，但这里必须截断成 byte，否则无法工作
                BYTE code = (BYTE)wParam;

                if (code == HSHELL_WINDOWACTIVATED
                    || code == HSHELL_WINDOWREPLACED
                    || code == HSHELL_WINDOWREPLACING
                    || (code == HSHELL_WINDOWDESTROYED && (HWND)lParam == $instance->_hwndSrc)
                ) {
                    $instance = nullptr;
                }
            } else if (message == $instance->_WM_NEWCURSOR) {
                // 来自 CursorHook 的消息
                // HCURSOR 似乎是共享资源，尽管来自别的进程但可以直接使用
                $instance->_cursorManager->AddHookCursor((HCURSOR)wParam, (HCURSOR)lParam);
            } else {
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        }
        return 0;
    }

    // 注册全屏窗口类
    void _RegisterHostWndClass() const {
        WNDCLASSEX wcex = {};
        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.lpfnWndProc = _HostWndProc;
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
        Debug::ThrowIfWin32Failed(_hwndHost, L"创建全屏窗口失败");

        // 设置窗口不透明
        Debug::ThrowIfWin32Failed(
            SetLayeredWindowAttributes(_hwndHost, 0, 255, LWA_ALPHA),
            L"SetLayeredWindowAttributes 失败"
        );
    }

    void _RegisterHookMsg() {
        _WM_SHELLHOOKMESSAGE = RegisterWindowMessage(L"SHELLHOOK");
        Debug::ThrowIfWin32Failed(
            _WM_SHELLHOOKMESSAGE,
            L"RegisterWindowMessage 失败"
        );
        Debug::ThrowIfWin32Failed(
            RegisterShellHookWindow(_hwndHost),
            L"RegisterShellHookWindow 失败"
        );
    }


    // 全屏窗口类名
    static constexpr const wchar_t* _HOST_WINDOW_CLASS_NAME = L"Window_Magpie_967EB565-6F73-4E94-AE53-00CC42592A22";

    // 指定为最大帧率时使用的渲染消息
    static constexpr const UINT _WM_MAXFRAMERATE = WM_USER;

    
    UINT _WM_NEWCURSOR = RegisterWindowMessage(L"MAGPIE_WM_NEWCURSOR");

    UINT _WM_SHELLHOOKMESSAGE{};

    HINSTANCE _hInst;
    HWND _hwndHost = NULL;
    HWND _hwndSrc;
    RECT _srcRect{};
    UINT _frameRate;

    ComPtr<IWICImagingFactory2> _wicImgFactory = nullptr;
    std::unique_ptr<EffectRenderer> _effectRenderer = nullptr;
    std::unique_ptr<WindowCapturerBase> _windowCapturer = nullptr;
    std::unique_ptr<CursorManager> _cursorManager = nullptr;
};
