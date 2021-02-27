// DllMain.cpp : Runtime.dll 的入口点。
//


#include "pch.h"
#include "Utils.h"
#include "KeyBoardHook.h"
#include "MagWindow.h"
using namespace D2D1;

HINSTANCE hInstance = NULL;
std::unique_ptr<MagWindow> magWnd = nullptr;

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
        magWnd.reset(new MagWindow(hInstance, GetForegroundWindow(), frameRate, effectsJson, false));
    } catch(...) {
        return FALSE;
    }
   
    return TRUE;
}

API_DECLSPEC BOOL WINAPI HasMagWindow() {
    return magWnd != nullptr;
}

API_DECLSPEC void WINAPI DestroyMagWindow() {
    magWnd = nullptr;
}


// 编译为 exe 时使用
int APIENTRY wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow) {

    Debug::ThrowIfFalse(
        SetProcessDPIAware(),
        L"SetProcessDPIAware 失败"
    );
    
    KeyBoardHook::hook({ VK_LMENU, VK_RMENU, VK_F11 });
    KeyBoardHook::setKeyDownCallback([=](int key) {
        std::vector<int> keys = KeyBoardHook::getPressedKeys();
        if ((std::find(keys.begin(), keys.end(), VK_LMENU) != keys.end() || std::find(keys.begin(), keys.end(), VK_RMENU) != keys.end())
            && std::find(keys.begin(), keys.end(), VK_F11) != keys.end()
        ) {
            if (magWnd == nullptr) {
                magWnd.reset(new MagWindow(hInstance, GetForegroundWindow(), 70, LR"(
[
  {
    "effect": "scale",
    "type": "Anime4KxDenoise"
  },
  {
    "effect": "scale",
    "type": "HQBicubic",
    "scale": [0,0],
    "sharpness": 1
  }
])", false));
            } else {
                magWnd = nullptr;
            }
        }
    });
    
    // 主消息循环
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
