// pch.h: 这是预编译标头文件。
// 下方列出的文件仅编译一次，提高了将来生成的生成性能。
// 这还将影响 IntelliSense 性能，包括代码完成和许多代码浏览功能。
// 但是，如果此处列出的文件中的任何一个在生成之间有更新，它们全部都将被重新编译。
// 请勿在此处添加要频繁更新的文件，这将使得性能优势无效。

#ifndef PCH_H
#define PCH_H

#include "framework.h"
#include "ErrorMessages.h"


#define XML(X) TEXT(#X)

#define API_DECLSPEC extern "C" __declspec(dllexport)


using namespace std::literals::string_literals;
using namespace Microsoft::WRL;
using namespace DirectX;


static std::string MakeWin32ErrorMsg(std::string_view msg) {
	return fmt::format("{}\n\tLastErrorCode：{}", msg, GetLastError());
}

static std::string MakeComErrorMsg(std::string_view msg, HRESULT hr) {
	return fmt::sprintf("%s\n\tHRESULT：0x%X", msg, hr);
}


#endif //PCH_H
