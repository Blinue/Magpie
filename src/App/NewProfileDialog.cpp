#include "pch.h"
#include "NewProfileDialog.h"
#if __has_include("NewProfileDialog.g.cpp")
#include "NewProfileDialog.g.cpp"
#endif
#include "Win32Utils.h"


namespace winrt::Magpie::App::implementation {

bool IsCandidateWindow(HWND hWnd) {
    if (!IsWindowVisible(hWnd)) {
        return false;
    }

    if (GetAncestor(hWnd, GA_ROOTOWNER) != hWnd) {
        return false;
    }

    // The window must not be cloaked by the shell
    UINT isCloaked{};
    DwmGetWindowAttribute(hWnd, DWMWA_CLOAKED, &isCloaked, sizeof(isCloaked));
    if (isCloaked == DWM_CLOAKED_SHELL) {
        return false;
    }

    RECT frameRect;
    if (!Win32Utils::GetWindowFrameRect(hWnd, frameRect)) {
        return false;
    }

    SIZE frameSize = Win32Utils::GetSizeOfRect(frameRect);
    if (frameSize.cx < 50 && frameSize.cy < 50) {
        return false;
    }

    return true;
}

std::vector<HWND> GetDesktopWindows() {
    std::vector<HWND> windows;
    windows.reserve(1024);

    HWND childWindow = NULL;
    for (size_t i = 0; i < 10000; ++i) {
        childWindow = FindWindowEx(NULL, childWindow, NULL, NULL);
        if (!childWindow) {
            break;
        }

        windows.push_back(childWindow);
    }

    return windows;
}

NewProfileDialog::NewProfileDialog() {
    InitializeComponent();

    std::vector<IInspectable> candidateWindows;

    std::vector<HWND> desktopWindows = GetDesktopWindows();
    for (HWND hWnd : desktopWindows) {
        if (IsCandidateWindow(hWnd)) {
            std::wstring title = Win32Utils::GetWndTitle(hWnd);
            if (title.empty()) {
                continue;
            }

            candidateWindows.emplace_back(box_value(title));
        }
    }

    _candidateWindows = single_threaded_observable_vector(std::move(candidateWindows));
}

}
