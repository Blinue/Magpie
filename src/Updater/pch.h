#pragma once

#include "targetver.h"

// Windows 头文件
#include <windows.h>

// C++ 运行时头文件
#include <cstdlib>
#include <vector>
#include <string>
#include <string_view>
#include <cassert>
#include <span>
#include <optional>
#include <filesystem>

// WIL
#include <wil/resource.h>
#include <wil/win32_helpers.h>
#include <wil/filesystem.h>
// wil::string_maker<std::wstring> 需要启用异常，应最后包含
#define WIL_ENABLE_EXCEPTIONS
// 防止编译失败
#define RESOURCE_SUPPRESS_STL
#include <wil/stl.h>
#undef RESOURCE_SUPPRESS_STL
#undef WIL_ENABLE_EXCEPTIONS
