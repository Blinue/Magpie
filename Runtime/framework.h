#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容
#define NOMINMAX	// 使用标准库的 min 和 max 而不是宏

// Windows 头文件
#include <windows.h>
#include <windowsx.h>
#include <magnification.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <dxgi1_5.h>
#include <wrl.h>
#include <dwmapi.h>

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
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.h>

// format
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
