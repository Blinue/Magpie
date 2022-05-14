#pragma once

// pch.h: 这是预编译标头文件。
// 下方列出的文件仅编译一次，提高了将来生成的生成性能。
// 这还将影响 IntelliSense 性能，包括代码完成和许多代码浏览功能。
// 但是，如果此处列出的文件中的任何一个在生成之间有更新，它们全部都将被重新编译。
// 请勿在此处添加要频繁更新的文件，这将使得性能优势无效。


#include <SDKDDKVer.h>

// Windows 头文件

#define WIN32_LEAN_AND_MEAN
#define WINRT_LEAN_AND_MEAN

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

// C++ 运行时头文件
#include <cstdlib>

// C++/WinRT 头文件
#include <unknwn.h>
#include <restrictederrorinfo.h>
#include <hstring.h>
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include <winrt/Windows.UI.ViewManagement.h>
