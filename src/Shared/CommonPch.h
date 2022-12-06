#pragma once

// Windows 头文件
#include <SDKDDKVer.h>
#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#include <ShlObj.h>
#include <shellapi.h>
#include <Shlwapi.h>

// 修复 C++/WinRT 头文件的警告
#undef GetCurrentTime
#undef GetNextSibling

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
#include <winrt/Windows.Foundation.Metadata.h>
#include <winrt/Windows.System.h>


namespace winrt {
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Foundation::Metadata;
using namespace Windows::System;
}

// 确保已编译 CONAN 依赖
#if !__has_include(<fmt/format.h>)
static_assert(false, "Build CONAN_INSTALL first!")
#endif

// fmt
#include <fmt/format.h>
#include <fmt/xchar.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Shlwapi.lib")

using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace std::chrono_literals;

// 导入 winrt 命名空间的 co_await 重载
// https://devblogs.microsoft.com/oldnewthing/20191219-00/?p=103230
using winrt::operator co_await;

// 宏定义

#define DEFINE_FLAG_ACCESSOR(Name, FlagBit, FlagsVar) \
	bool Name() const noexcept { return FlagsVar & FlagBit; } \
	void Name(bool value) noexcept { \
		if (value) { \
			FlagsVar |= FlagBit; \
		} else { \
			FlagsVar &= ~FlagBit; \
		} \
	}
