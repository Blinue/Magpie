// DllMain.cpp : Runtime.dll 的入口点。
//


#include "pch.h"
#include "MagWindow.h"


HINSTANCE hInstance = NULL;


// DLL 入口
BOOL APIENTRY DllMain(
    HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        hInstance = hModule;
        break;
    case DLL_PROCESS_DETACH:
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}


API_DECLSPEC bool WINAPI CreateMagWindow(
    const wchar_t* effectsJson,
    int captureMode,
    bool showFPS,
    bool lowLatencyMode,
    bool noVSync,
    bool noDisturb
) {
    try {
        HWND hwnd = GetForegroundWindow();
        Debug::ThrowIfWin32Failed(
            hwnd,
            L"GetForegroundWindow 返回 NULL"
        );

        MagWindow::CreateInstance(hInstance, hwnd, effectsJson, captureMode, showFPS, lowLatencyMode, noVSync, noDisturb);
    } catch(const magpie_exception&) {
        return FALSE;
    } catch (...) {
        Debug::WriteErrorMessage(L"创建全屏窗口发生未知错误");
        return FALSE;
    }

    return TRUE;
}

API_DECLSPEC bool WINAPI HasMagWindow() {
    return MagWindow::$instance != nullptr;
}

API_DECLSPEC void WINAPI DestroyMagWindow() {
    MagWindow::$instance = nullptr;
}


API_DECLSPEC const WCHAR* WINAPI GetLastErrorMsg() {
    return Debug::GetLastErrorMessage().c_str();
}


API_DECLSPEC HWND WINAPI GetSrcWnd() {
    if (MagWindow::$instance == nullptr) {
        return NULL;
    }

    return MagWindow::$instance->GetSrcWnd();
}


API_DECLSPEC HWND WINAPI GetHostWnd() {
    if (MagWindow::$instance == nullptr) {
        return NULL;
    }

    return MagWindow::$instance->GetHostWnd();
}
