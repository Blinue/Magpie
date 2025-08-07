#pragma once

// pch.h: 这是预编译标头文件。
// 下方列出的文件仅编译一次，提高了将来生成的生成性能。
// 这还将影响 IntelliSense 性能，包括代码完成和许多代码浏览功能。
// 但是，如果此处列出的文件中的任何一个在生成之间有更新，它们全部都将被重新编译。
// 请勿在此处添加要频繁更新的文件，这将使得性能优势无效。


// Windows
#include <SDKDDKVer.h>
#include <Windows.h>
#include <windowsx.h>

// C++
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>
#include <cassert>

using namespace std::string_literals;
using namespace std::string_view_literals;
