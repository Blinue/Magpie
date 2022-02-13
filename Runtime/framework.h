#pragma once

#include "targetver.h"

// Windows 头文件

// 从 Windows 头文件中排除极少使用的内容
#define WIN32_LEAN_AND_MEAN
// 使用标准库的 min 和 max 而不是宏
#define NOMINMAX
// 排除不需要的 API
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
#define NOWH
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
