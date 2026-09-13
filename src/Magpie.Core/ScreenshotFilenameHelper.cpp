#include "pch.h"
#include "ScreenshotFilenameHelper.h"
#include "SmallVector.h"

namespace Magpie {

enum class TemplateUnitType : uint8_t {
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

struct TemplateUnit {
	TemplateUnitType type = TemplateUnitType::Character;
	// Character: 表示 UTF-8 字符码
	// WindowTitle 和 ProcessName: 表示最大字符数量，非正值为无限制
	// 其他类型不使用
	int8_t value = 0;
};

static TemplateUnit GetNextUnit(std::string_view& str) noexcept {
	assert(!str.empty());

	const char curChar = str[0];
	
	if (curChar != '%' || str.size() < 3) {
		str.remove_prefix(1);
		return { TemplateUnitType::Character, (int8_t)curChar };
	}

	const char nextChar = str[1];

	if (str[2] == '%') {
		switch (nextChar) {
		case 'Y':
			str.remove_prefix(3);
			return { TemplateUnitType::Year, 0 };
		case 'y':
			str.remove_prefix(3);
			return { TemplateUnitType::Year2, 0 };
		case 'm':
			str.remove_prefix(3);
			return { TemplateUnitType::Month, 0 };
		case 'D':
			str.remove_prefix(3);
			return { TemplateUnitType::Day, 0 };
		case 'H':
			str.remove_prefix(3);
			return { TemplateUnitType::Hour, 0 };
		case 'M':
			str.remove_prefix(3);
			return { TemplateUnitType::Minite, 0 };
		case 'S':
			str.remove_prefix(3);
			return { TemplateUnitType::Second, 0 };
		default:
			str.remove_prefix(1);
			return { TemplateUnitType::Character, (int8_t)curChar };
		}
	}

	// 最大的字符数量限制
	constexpr uint8_t MAX_CHAR_COUNT_LIMIT = 99;

	// 检查 %WT[:nn]% 和 %PN[:nn]%
	if (str.size() >= 4 && ((nextChar == 'W' && str[2] == 'T') || (nextChar == 'P' && str[2] == 'N'))) {
		if (str[3] == '%') {
			str.remove_prefix(4);
			return { nextChar == 'W' ? TemplateUnitType::WindowTitle :
				TemplateUnitType::ProcessName, 0 };
		} else if (str[3] == ':') {
			uint8_t limit;
			auto result = std::from_chars(str.data() + 4, str.data() + str.size(), limit);

			if (result.ec == std::errc{} && limit <= MAX_CHAR_COUNT_LIMIT &&
				result.ptr != str.data() + str.size() && *result.ptr == '%')
			{
				str.remove_prefix(result.ptr - str.data() + 1);
				return { nextChar == 'W' ? TemplateUnitType::WindowTitle :
					TemplateUnitType::ProcessName, (int8_t)limit };
			}
		}
	}

	str.remove_prefix(1);
	return { TemplateUnitType::Character, (int8_t)curChar };
}

static bool IsValidUnit(TemplateUnit unit) noexcept {
	// 不允许无法作为文件名的特殊字符，禁止的字符列表来自
	// https://learn.microsoft.com/en-us/windows/win32/fileio/naming-a-file#naming-conventions
	if (unit.type == TemplateUnitType::Character) {
		constexpr std::string_view FORBIDDEN_LIST = "<>:\"/\\|?*";
		return FORBIDDEN_LIST.find(unit.value) == std::string_view::npos;
	} else {
		return true;
	}
}

bool ScreenshotFilenameHelper::IsValidTemplate(std::string_view templateStr) noexcept {
	// 如果 WT 和 PN 不使用冒号，这里可以直接检查字符串是否包含禁止的字符，
	// 但我找不到比冒号更合适的符号了。
	while (!templateStr.empty()) {
		if (!IsValidUnit(GetNextUnit(templateStr))) {
			return false;
		}
	}

	return true;
}

bool ScreenshotFilenameHelper::GenerateFilename(
	std::string_view templateStr,
	const std::filesystem::path& screenshotDir,
	HWND hwndSrc
) noexcept {
	SmallVector<TemplateUnit> units;

	bool hasWindowTitleUnit = false;
	bool hasProcessNameUnit = false;
	bool hasDateUnit = false;

	while (!templateStr.empty()) {
		TemplateUnit unit = GetNextUnit(templateStr);

		if (!IsValidUnit(unit)) {
			return false;
		}

		if (unit.type == TemplateUnitType::WindowTitle) {
			hasWindowTitleUnit = true;
		} else if (unit.type == TemplateUnitType::ProcessName) {
			hasProcessNameUnit = true;
		} else if (unit.type != TemplateUnitType::Character) {
			hasDateUnit = true;
		}

		units.push_back(std::move(unit));
	}

	std::string windowTitle;
	std::string processName;

	if (hasWindowTitleUnit) {

	}
	
	return false;
}

}
