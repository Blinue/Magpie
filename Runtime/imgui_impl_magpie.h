#pragma once
#include "pch.h"


bool ImGui_ImplMagpie_Init();
void ImGui_ImplMagpie_Shutdown();
void ImGui_ImplMagpie_NewFrame();

LRESULT ImGui_ImplMagpie_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
