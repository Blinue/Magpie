// pch.h: 这是预编译标头文件。
// 下方列出的文件仅编译一次，提高了将来生成的生成性能。
// 这还将影响 IntelliSense 性能，包括代码完成和许多代码浏览功能。
// 但是，如果此处列出的文件中的任何一个在生成之间有更新，它们全部都将被重新编译。
// 请勿在此处添加要频繁更新的文件，这将使得性能优势无效。

#ifndef PCH_H
#define PCH_H


#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容
// Windows 头文件
#include <windows.h>
#include <wrl.h>
#include <d2d1_3.h>
#include <d2d1effects_2.h>

// C++ 运行时头文件
#include <string>

#pragma comment(lib, "d2d1.lib")


#define XML(X) TEXT(#X)

#define API_DECLSPEC extern "C" __declspec(dllexport)

using namespace Microsoft::WRL;
using namespace std::literals::string_literals;


#endif //PCH_H
