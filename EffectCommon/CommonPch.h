#pragma once


#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容
// Windows 头文件
#include <windows.h>
#include <wrl.h>
#include <d2d1_3.h>
#include <d2d1effects_2.h>

// C++ 运行时头文件
#include <string>
#include <cassert>

#pragma comment(lib, "d2d1.lib")

#include "CommonDebug.h"
#include "EffectUtils.h"

#define XML(X) TEXT(#X)

#define API_DECLSPEC extern "C" __declspec(dllexport)

using namespace Microsoft::WRL;
using namespace std::literals::string_literals;

