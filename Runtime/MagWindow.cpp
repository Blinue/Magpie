#include "pch.h"
#include "MagWindow.h"

std::unique_ptr<MagWindow> MagWindow::$instance = nullptr;

UINT MagWindow::_WM_NEWCURSOR32 = RegisterWindowMessage(L"MAGPIE_WM_NEWCURSOR32");
UINT MagWindow::_WM_NEWCURSOR64 = RegisterWindowMessage(L"MAGPIE_WM_NEWCURSOR64");
UINT MagWindow::_WM_DESTORYMAG = RegisterWindowMessage(L"MAGPIE_WM_DESTORYMAG");
