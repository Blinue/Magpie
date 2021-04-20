#pragma once
#include "pch.h"
#include "Utils.h"
#include "D2DContext.h"
#include "MagCallbackWindowCapturer.h"
#include "GDIWindowCapturer.h"
#include "WinRTCapturer.h"
#include "CursorManager.h"
#include "FrameCatcher.h"
#include "RenderManager.h"


class MagWindow {
public:
    // 确保只能同时存在一个全屏窗口
    static std::unique_ptr<MagWindow> $instance;

    static void CreateInstance(
        HINSTANCE hInstance,
        HWND hwndSrc,
        const std::wstring_view& effectsJson,
        int captureMode = 0,
        bool showFPS = false,
        bool lowLatencyMode = false,
        bool noVSync = false,
        bool noDisturb = false
    ) {
        $instance.reset(new MagWindow(
            hInstance,
            hwndSrc,
            effectsJson,
            captureMode,
            showFPS,
            lowLatencyMode,
            noVSync,
            noDisturb
        ));
    }
    
    // 不可复制，不可移动
    MagWindow(const MagWindow&) = delete;
    MagWindow(MagWindow&&) = delete;
    
    ~MagWindow() {
        // 以下面的顺序释放资源
        _renderManager = nullptr;
        _windowCapturer = nullptr;
        _d2dContext = nullptr;

        DestroyWindow(_hwndHost);
        UnregisterClass(_HOST_WINDOW_CLASS_NAME, _hInst);

        CoUninitialize();

        PostQuitMessage(0);
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
        const std::wstring_view& effectsJson,
        int captureMode,
        bool showFPS,
        bool lowLatencyMode,
        bool noVSync,
        bool noDisturb
    ) : _hInst(hInstance), _hwndSrc(hwndSrc) {
        if ($instance) {
            Debug::Assert(false, L"已存在全屏窗口");
        }

        Debug::ThrowIfComFailed(
            CoInitializeEx(NULL, COINIT_MULTITHREADED),
            L"初始化 COM 出错"
        );

        Debug::Assert(
            IsWindow(_hwndSrc) && IsWindowVisible(_hwndSrc),
            L"hwndSrc 不合法"
        );

        Debug::Assert(
            captureMode >= 0 && captureMode <= 2,
            L"非法的抓取模式"
        );

        Utils::GetClientScreenRect(_hwndSrc, _srcClient);

        _RegisterHostWndClass();
        _CreateHostWnd(noDisturb);

        Utils::GetClientScreenRect(_hwndHost, _hostClient);

        Debug::ThrowIfComFailed(
            CoCreateInstance(
                CLSID_WICImagingFactory,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(&_wicImgFactory)
            ),
            L"创建 WICImagingFactory 失败"
        );


        // 初始化 D2DContext
        _d2dContext.reset(new D2DContext(
            hInstance,
            _hwndHost,
            _hostClient,
            lowLatencyMode,
            noVSync,
            noDisturb
        ));

        if (captureMode == 0) {
            _windowCapturer.reset(new WinRTCapturer(
                *_d2dContext,
                _hwndSrc,
                _srcClient
            ));
        } else if (captureMode == 1) {
            _windowCapturer.reset(new GDIWindowCapturer(
                *_d2dContext,
                _hwndSrc,
                _srcClient,
                _wicImgFactory.Get()
            ));
        } else {
            _windowCapturer.reset(new MagCallbackWindowCapturer(
                *_d2dContext,
                hInstance,
                _hwndHost,
                _srcClient
            ));
        }

        _renderManager.reset(new RenderManager(
            *_d2dContext,
            effectsJson,
            _srcClient,
            _hostClient,
            noDisturb
        ));

        // 初始化 CursorManager
        _renderManager->AddCursorManager(_hInst, _wicImgFactory.Get());

        if (showFPS) {
            // 初始化 FrameCatcher
            _renderManager->AddFrameCatcher();
        }

        ShowWindow(_hwndHost, SW_NORMAL);

        PostMessage(_hwndHost, _WM_RENDER, 0, 0);

        // 接收 Shell 消息有时不可靠
        // _RegisterHookMsg();
    }

    // 关闭全屏窗口并退出线程
    // 有两个退出路径：
    // 1. 前台窗口发生改变
    // 2. 收到_WM_DESTORYMAG 消息
    static void _DestroyMagWindow() {
        $instance = nullptr;
    }

    // 渲染一帧，不抛出异常
    void _Render() {
        try {
            const auto& frame = _windowCapturer->GetFrame();
            if (frame) {
                _renderManager->Render(frame);
            }
        } catch (const magpie_exception& e) {
            Debug::WriteErrorMessage(L"渲染失败：" + e.what());
        } catch (...) {
            Debug::WriteErrorMessage(L"渲染出现未知错误");
        }
    }

    LRESULT _HostWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
        switch (message) {
        case _WM_RENDER:
        {
            // 前台窗口改变时自动关闭全屏窗口
            // 接收 shell 消息有时不可靠
            if (GetForegroundWindow() != _hwndSrc) {
                _DestroyMagWindow();
                break;
            }

            _Render();

            // 立即渲染下一帧
            // 垂直同步开启时自动限制帧率
            if (!PostMessage(hWnd, _WM_RENDER, 0, 0)) {
                Debug::WriteErrorMessage(L"PostMessage 失败");
            }
            break;
        }
        default:
        {
            if (message == _WM_NEWCURSOR32) {
                // 来自 CursorHook 的消息
                // HCURSOR 似乎是共享资源，尽管来自别的进程但可以直接使用
                // 
                // 如果消息来自 32 位进程，本程序为 64 位，必须转换为补符号位扩展，这是为了和 SetCursor 的处理方法一致
                // SendMessage 为补 0 扩展，SetCursor 为补符号位扩展
                _renderManager->AddHookCursor((HCURSOR)(INT_PTR)(INT32)wParam, (HCURSOR)(INT_PTR)(INT32)lParam);
            } else if (message == _WM_NEWCURSOR64) {
                // 如果消息来自 64 位进程，本程序为 32 位，HCURSOR 会被截断
                // Q: 如果被截断是否能正常工作？
                _renderManager->AddHookCursor((HCURSOR)wParam, (HCURSOR)lParam);
            } else if (message == _WM_DESTORYMAG) {
                _DestroyMagWindow();
            } else {
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        }

        return 0;
    }

    static LRESULT CALLBACK _HostWndProcStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
        if (!$instance) {
            return DefWindowProc(hWnd, message, wParam, lParam);
        } else {
            return $instance->_HostWndProc(hWnd, message, wParam, lParam);
        }
    }

    // 注册全屏窗口类
    void _RegisterHostWndClass() const {
        WNDCLASSEX wcex = {};
        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.lpfnWndProc = _HostWndProcStatic;
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
    static constexpr const UINT _WM_RENDER = WM_USER;
    static UINT _WM_NEWCURSOR32;
    static UINT _WM_NEWCURSOR64;
    static UINT _WM_DESTORYMAG;

    UINT _WM_SHELLHOOKMESSAGE{};

    HINSTANCE _hInst;
    HWND _hwndHost = NULL;
    HWND _hwndSrc;
    RECT _srcClient{};
    RECT _hostClient{};

    ComPtr<IWICImagingFactory2> _wicImgFactory = nullptr;
    std::unique_ptr<D2DContext> _d2dContext = nullptr;
    std::unique_ptr<WindowCapturerBase> _windowCapturer = nullptr;
    std::unique_ptr<RenderManager> _renderManager = nullptr;
};
