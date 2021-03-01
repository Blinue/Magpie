// DllMain.cpp : Runtime.dll 的入口点。
//


#include "pch.h"
#include "Utils.h"
#include "KeyBoardHook.h"
#include "MagWindow.h"

#undef NTDDI_VERSION
#undef _WIN32_WINNT
#include "easyhook.h"

using namespace D2D1;

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


API_DECLSPEC BOOL WINAPI CreateMagWindow(
    UINT frameRate,
    const wchar_t* effectsJson,
    bool noDisturb
) {
    try {
        HWND hwnd = GetForegroundWindow();
        Debug::ThrowIfFalse(
            hwnd,
            L"GetForegroundWindow 返回 NULL"
        );

        MagWindow::CreateInstance(hInstance, hwnd, frameRate, effectsJson, noDisturb);
    } catch(...) {
        Debug::writeLine(L"创建 MagWindow 失败");
        return FALSE;
    }
    
    return TRUE;
}

API_DECLSPEC BOOL WINAPI HasMagWindow() {
    return MagWindow::$instance != nullptr;
}

API_DECLSPEC void WINAPI DestroyMagWindow() {
    MagWindow::$instance = nullptr;
}
