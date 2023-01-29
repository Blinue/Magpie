// header.h: 标准系统包含文件的包含文件，
// 或特定于项目的包含文件
//

#pragma once

#include <SDKDDKVer.h>

#define WIN32_LEAN_AND_MEAN

// Windows 头文件

// 从 windows.h 里排除不需要的 API
#define NOGDICAPMASKS
#define NOVIRTUALKEYCODE
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICONS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
#define OEMRESOURCE
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NOCTLMGR
#define NODRAWTEXT
#define NOGDI
#define NOMEMMGR
#define NOMETAFILE
#define NOMINMAX
#define NOMSG
#define NOOPENFILE
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX

#include <windows.h>

// C++ 运行时头文件
#include <cstdlib>
#include <vector>
#include <string>
#include <string_view>
#include <cassert>

// 确保已编译 CONAN 依赖
#if !__has_include(<fmt/format.h>)
static_assert(false, "Build CONAN_INSTALL first!")
#endif

// fmt
#include <fmt/format.h>
#include <fmt/xchar.h>
