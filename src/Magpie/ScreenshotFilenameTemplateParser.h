#pragma once
#include "ScalingOptions.h"

namespace Magpie {

struct ScreenshotFilenameTemplateParser {
	// 空字符串视为合法
	static bool IsValid(std::string_view str) noexcept;

	// 失败时返回空集合
	static bool Parse(std::string_view str, ScreenshotFilenameTemplate& result) noexcept;
};

}
