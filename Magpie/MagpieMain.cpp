// MagpieMain.cpp : 定义应用程序的入口点。
//


#include "pch.h"
#include "Utils.h"
#include "KeyBoardHook.h"
#include "MagWindow.h"
using namespace D2D1;


std::unique_ptr<MagWindow> magWnd = nullptr;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
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
