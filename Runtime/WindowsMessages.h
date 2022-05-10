#pragma once
#include "pch.h"


// 管理所有自定义消息
struct WindowsMessages {
	// 下面的消息保证在操作系统中唯一
	inline static const UINT WM_DESTORYHOST = RegisterWindowMessage(L"MAGPIE_WM_DESTORYHOST");
	inline static const UINT WM_TOGGLE_OVERLAY = RegisterWindowMessage(L"MAGPIE_WM_TOGGLE_OVERLAY");

	// 下面的消息内部使用
};
