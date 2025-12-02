// 供 C++ 和 RC 使用
#pragma once

#define _STRINGIFY_HELPER(x) #x
#define STRINGIFY(x) _STRINGIFY_HELPER(x)
#define _WIDEN_STRINGIFY_HELPER(x) L ## #x
#define WIDEN_STRINGIFY(x) _WIDEN_STRINGIFY_HELPER(x)
