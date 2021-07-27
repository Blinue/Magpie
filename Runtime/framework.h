// header.h: 标准系统包含文件的包含文件，
// 或特定于项目的包含文件
//

#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容
#define OEMRESOURCE	// 需要设置系统光标 https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setsystemcursor
#define NOMINMAX	// 使用标准库的min和max而不是宏

// Windows 头文件
#include <windows.h>
#include <windowsx.h>
#include <magnification.h>
#include <d2d1_3.h>
#include <d2d1effects_2.h>
#include <d3d11.h>
#include <dxgi1_6.h>
#include <dwrite_3.h>
#include <wrl.h>
#include <dwmapi.h>

// C++ 运行时头文件
#include <string>
#include <memory>
#include <cstdlib>
#include <exception>
#include <functional>
#include <algorithm>
#include <string_view>
#include <thread>
#include <atomic>
#include <mutex>

// C++/WinRT 头文件
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.h>


// format
#include <fmt/format.h>
#include <fmt/xchar.h>


#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "Magnification.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "windowsapp")
#pragma comment(lib, "dwmapi.lib")
