// header.h: 标准系统包含文件的包含文件，
// 或特定于项目的包含文件
//

#pragma once

#include <SDKDDKVer.h>

// Windows 头文件

#define NOGDI
#include <windows.h>

// C++ 运行时头文件
#include <cstdlib>
#include <vector>
#include <string>
#include <string_view>
#include <cassert>
#include <span>
#include <optional>

// string_maker<std::wstring> 需要启用异常
#define WIL_ENABLE_EXCEPTIONS
#include <wil/stl.h>
#undef WIL_ENABLE_EXCEPTIONS
#include <wil/resource.h>
#include <wil/win32_helpers.h>
#include <wil/filesystem.h>
