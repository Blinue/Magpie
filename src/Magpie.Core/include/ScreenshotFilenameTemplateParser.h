#pragma once
#include "SmallVector.h"

namespace Magpie {

enum class ScreenshotFilenameTemplateUnitType : uint8_t {
	Character,
	// %WT:nn%: 可限制字符数量，最大是 99
	WindowTitle,
	// %PN:nn%: 可限制字符数量，最大是 99
	ProcessName,
	// %Y%
	Year,
	// %y%: 年份的最后两位
	Year2,
	// %m%
	Month,
	// %D%
	Day,
	// %H%: 24 小时制
	Hour,
	// %M%
	Minite,
	// %S%
	Second
};

struct ScreenshotFilenameTemplateUnit {
	ScreenshotFilenameTemplateUnitType type = ScreenshotFilenameTemplateUnitType::Character;
	// Character: 表示 UTF-8 字符码
	// WindowTitle 和 ProcessName: 表示最大字符数量，非正值为无限制
	// 其他类型不使用
	int8_t value = 0;
};

struct ScreenshotFilenameTemplate {
	SmallVector<ScreenshotFilenameTemplateUnit> units;
};

struct ScreenshotFilenameTemplateParser {
	// 空字符串视为合法
	static bool IsValid(std::string_view str) noexcept;

	// 失败时返回空集合
	static bool Parse(std::string_view str, ScreenshotFilenameTemplate& result) noexcept;
};

}
