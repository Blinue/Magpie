#include "pch.h"
#include "KeyBoardHook.h"


std::map<int, bool> KeyBoardHook::_keyStates;
HHOOK KeyBoardHook::_hHook = NULL;
std::function<void(int)> KeyBoardHook::_downCallback;
std::function<void(int)> KeyBoardHook::_upCallback;
