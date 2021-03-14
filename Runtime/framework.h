// header.h: 标准系统包含文件的包含文件，
// 或特定于项目的包含文件
//

#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容
#define OEMRESOURCE	// 需要设置系统光标 https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setsystemcursor

// Windows 头文件
#include <windows.h>
#include <magnification.h>
#include <windowsx.h>
#include <d2d1_3.h>
#include <d2d1effects_2.h>
#include <d3d11.h>
#include <dxgi1_6.h>
#include <wrl.h>

// C 运行时头文件
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

// C++ 运行时头文件
#include <string>
#include <memory>
#include <cstdlib>
#include <exception>
#include <functional>
#include <algorithm>
#include <string_view>
