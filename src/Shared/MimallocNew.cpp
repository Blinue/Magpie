// 导入 mimalloc 提供的 new
// 只需包含 mimalloc-new-delete.h 一次即可全局重载

#include "pch.h"
#pragma warning(push)
#pragma warning(disable: 4100)
#include <mimalloc-new-delete.h>
#pragma warning(pop)
