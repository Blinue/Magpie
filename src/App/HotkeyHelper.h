#pragma once
#include "pch.h"
#include <winrt/Magpie.App.h>


namespace winrt {

// 将 HotkeyAction 映射为字符串
hstring to_hstring(Magpie::App::HotkeyAction action);

namespace Magpie::App {

std::string to_string(HotkeyAction action);

}

}