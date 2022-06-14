#pragma once

#include <SDKDDKVer.h>

#define WIN32_LEAN_AND_MEAN
#define WINRT_LEAN_AND_MEAN
#define WINRT_NO_MODULE_LOCK

// Windows 头文件

// 从 windows.h 里排除不需要的 API
#define NOMINMAX	// 使用 std::min 和 std::max 而不是宏
#define NOGDICAPMASKS
#define NOMENUS
#define NOICONS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define NOATOM
#define NOCLIPBOARD
#define NODRAWTEXT
#define NOMB
#define NOMEMMGR
#define NOMETAFILE
#define NOOPENFILE
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX

#include <windows.h>
#include <dwmapi.h>
#include <ShlObj.h>

// C++ 运行时头文件
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <functional>
#include <span>

// C++/WinRT 头文件
#include <unknwn.h>
#include <restrictederrorinfo.h>
#include <hstring.h>
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.System.h>


namespace winrt {
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::System;
}

// 确保已编译 CONAN 依赖
#if !__has_include(<fmt/format.h>)
static_assert(false, "Build CONAN_INSTALL first!")
#endif

// fmt
#include <fmt/format.h>
#include <fmt/xchar.h>
#include <fmt/printf.h>


#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "shell32.lib")

using namespace std::string_literals;
using namespace std::string_view_literals;


// 宏定义

// 发布时传入 MAGPIE_VERSION 宏指定版本号
#ifndef MAGPIE_VERSION
#define MAGPIE_VERSION "dev"
#endif // !MAGPIE_VERSION

#define MAGPIE_VERSION_W L"" MAGPIE_VERSION
