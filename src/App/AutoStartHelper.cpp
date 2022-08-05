#include "pch.h"
#include "AutoStartHelper.h"
#include <taskschd.h>
#include <Lmcons.h>
#include "Logger.h"
#include "StrUtils.h"
#include "Win32Utils.h"

#pragma comment(lib, "Taskschd.lib")

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 使用任务计划程序实现开机启动，优点是以管理员身份启动时不会显示 UAC
// 移植自 https://github.com/microsoft/PowerToys/blob/3d54cb838504c12f59516afaf1a00fde2dd5d01b/src/runner/auto_start_helper.cpp
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


static constexpr const DWORD USERNAME_DOMAIN_LEN = DNLEN + UNLEN + 2; // Domain Name + '\' + User Name + '\0'
static constexpr const DWORD USERNAME_LEN = UNLEN + 1; // User Name + '\0'

namespace winrt::Magpie::App {

bool AutoStartHelper::CreateAutoStartTask(bool runElevated) {
    WCHAR usernameDomain[USERNAME_DOMAIN_LEN];
    WCHAR username[USERNAME_LEN];

    // ------------------------------------------------------
    // Get the Domain/Username for the trigger.
    if (!GetEnvironmentVariable(L"USERNAME", username, USERNAME_LEN)) {
        Logger::Get().Win32Error("获取用户名失败");
        return false;
    }
    if (!GetEnvironmentVariable(L"USERDOMAIN", usernameDomain, USERNAME_DOMAIN_LEN)) {
        Logger::Get().Win32Error("获取用户域失败");
        return false;
    }

    wcscat_s(usernameDomain, L"\\");
    wcscat_s(usernameDomain, username);

    std::wstring taskName = StrUtils::ConcatW(L"Autorun for ", username);

    // Get the executable path passed to the custom action.
    WCHAR executablePath[MAX_PATH];
    GetModuleFileName(NULL, executablePath, MAX_PATH);

    // ------------------------------------------------------
    // Create an instance of the Task Service.
    com_ptr<ITaskService> taskService;
    HRESULT hr = CoCreateInstance(
        CLSID_TaskScheduler,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&taskService)
    );
    if (FAILED(hr)) {
        Logger::Get().ComError("创建 ITaskService 失败", hr);
        return false;
    }

    // Connect to the task service.
    hr = taskService->Connect(Win32Utils::Variant(), Win32Utils::Variant(), Win32Utils::Variant(), Win32Utils::Variant());
    if (FAILED(hr)) {
        Logger::Get().ComError("ITaskService::Connect 失败", hr);
        return false;
    }

    // ------------------------------------------------------
    // Get the Magpie task folder. Creates it if it doesn't exist.
    com_ptr<ITaskFolder> taskFolder;
    hr = taskService->GetFolder(StrUtils::BStr(L"\\Magpie"), taskFolder.put());
    if (FAILED(hr)) {
        // Folder doesn't exist. Get the Root folder and create the PowerToys subfolder.
        com_ptr<ITaskFolder> rootFolder = NULL;
        hr = taskService->GetFolder(StrUtils::BStr(L"\\"), rootFolder.put());
        if (FAILED(hr)) {
            Logger::Get().ComError("获取根目录失败", hr);
            return false;
        }
        
        hr = rootFolder->CreateFolder(StrUtils::BStr(L"\\Magpie"), Win32Utils::Variant(L""), taskFolder.put());
        if (FAILED(hr)) {
            Logger::Get().ComError("创建 Magpie 任务文件夹失败", hr);
            return false;
        }
    }

    return false;
}

bool AutoStartHelper::DeleteAutoStartTask() {
    return false;
}

bool AutoStartHelper::IsAutoStartTaskActive() {
    return false;
}

}