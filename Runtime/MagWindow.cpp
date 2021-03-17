#include "pch.h"
#include "MagWindow.h"

std::unique_ptr<MagWindow> MagWindow::$instance = nullptr;

UINT MagWindow::_WM_NEWCURSOR = RegisterWindowMessage(L"MAGPIE_WM_NEWCURSOR");
