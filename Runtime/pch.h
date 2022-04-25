#pragma once

// pch.h: 这是预编译标头文件。
// 下方列出的文件仅编译一次，提高了将来生成的生成性能。
// 这还将影响 IntelliSense 性能，包括代码完成和许多代码浏览功能。
// 但是，如果此处列出的文件中的任何一个在生成之间有更新，它们全部都将被重新编译。
// 请勿在此处添加要频繁更新的文件，这将使得性能优势无效。


#include "targetver.h"

// Windows 头文件

// 从 windows.h 里排除不需要的 API
#define WIN32_LEAN_AND_MEAN	// 排除极少使用的内容
#define NOMINMAX	// 使用 std::min 和 std::max 而不是宏
#define NOGDICAPMASKS
#define NOMENUS
#define NOICONS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NODRAWTEXT
#define NOMB
#define NOMEMMGR
#define NOMETAFILE
#define NOOPENFILE
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOTEXTMETRIC
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX

#include <windows.h>
#include <windowsx.h>
#include <magnification.h>
#include <wrl/client.h>
#include <dwmapi.h>
#include <profileapi.h>
#include <psapi.h>

// DirectX 头文件
#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>

// C++ 运行时头文件
#include <string>
#include <memory>
#include <cstdlib>
#include <functional>
#include <algorithm>
#include <string_view>
#include <span>

// C++/WinRT 头文件
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>

// fmt
#include <fmt/format.h>
#include <fmt/xchar.h>
#include <fmt/printf.h>

// spdlog
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>


#pragma comment(lib, "Magnification.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "windowsapp")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "psapi.lib")


using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;
