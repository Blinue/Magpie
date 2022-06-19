#include "pch.h"
#include "HotkeyManager.h"
#if __has_include("HotkeyManager.g.cpp")
#include "HotkeyManager.g.cpp"
#endif


namespace winrt::Magpie::App::implementation {

bool HotkeyManager::IsError(HotkeyAction action) {
	return false;
}

}
