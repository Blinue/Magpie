#include "pch.h"
#include "MagApp.h"


MagApp::~MagApp() {}

bool MagApp::Run(HWND hwndSrc, winrt::Magpie::Runtime::MagSettings const& settings) {
    _hwndSrc = hwndSrc;
    _settings = settings;

    return false;
}

void MagApp::Stop() {
    if (_hwndDDF) {
        DestroyWindow(_hwndDDF);
    }
    if (_hwndHost) {
        DestroyWindow(_hwndHost);
    }
}

MagApp::MagApp() {}

void MagApp::_RunMessageLoop() {
}

void MagApp::_RegisterWndClasses() const {
}

bool MagApp::_CreateHostWnd() {
    return false;
}

bool MagApp::_InitFrameSource(int captureMode) {
    return false;
}

bool MagApp::_DisableDirectFlip() {
    return false;
}

LRESULT MagApp::_HostWndProcStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    return LRESULT();
}

LRESULT MagApp::_HostWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    return LRESULT();
}

void MagApp::_OnQuit() {
}
