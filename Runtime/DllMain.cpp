// DllMain.cpp : Runtime.dll 的入口点。
//


#include "pch.h"
#include "MagWindow.h"
#include "Env.h"
#include <spdlog/spdlog.h>

HINSTANCE hInst = NULL;


// DLL 入口
BOOL APIENTRY DllMain(
    HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        hInst = hModule;
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


API_DECLSPEC void WINAPI RunMagWindow(
    void reportStatus(int status, const wchar_t* errorMsg),
    HWND hwndSrc,
    const char* scaleModel,
    int captureMode,
    bool showFPS,
    bool noDisturb
) {
    Debug::ThrowIfComFailed(
        CoInitializeEx(NULL, COINIT_MULTITHREADED),
        L"初始化 COM 出错"
    );

    try {
        Debug::Assert(
            IsWindow(hwndSrc) && IsWindowVisible(hwndSrc) && Utils::GetWindowShowCmd(hwndSrc) == SW_NORMAL,
            L"不合法的源窗口"
        );
        Debug::Assert(
            captureMode >= 0 && captureMode <= 1,
            L"非法的抓取模式"
        );

        Env::CreateInstance(hInst, hwndSrc, scaleModel, captureMode, showFPS, noDisturb);
        MagWindow::CreateInstance();
    } catch(const magpie_exception& e) {
        reportStatus(0, (L"创建全屏窗口出错：" + e.what()).c_str());
        return;
    } catch (...) {
        Debug::WriteErrorMessage(L"创建全屏窗口发生未知错误");
        reportStatus(0, L"未知错误");
        return;
    }
    
    reportStatus(2, nullptr);

    // 主消息循环
    std::wstring errMsg = MagWindow::RunMsgLoop();

    Env::$instance = nullptr;
    reportStatus(0, errMsg.empty() ? nullptr : errMsg.c_str());
}
