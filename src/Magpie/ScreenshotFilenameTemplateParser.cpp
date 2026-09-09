#include "pch.h"
#include "ScreenshotFilenameTemplateParser.h"

namespace Magpie {

static ScreenshotFilenameTemplateUnit GetNextUnit(std::string_view& str) noexcept {
	assert(!str.empty());

	const char curChar = str[0];
	
	if (curChar != '%' || str.size() < 3) {
		str.remove_prefix(1);
		return { ScreenshotFilenameTemplateUnitType::Character, (int8_t)curChar };
	}

	const char nextChar = str[1];

	if (str[2] == '%') {
		switch (nextChar) {
		case 'Y':
			str.remove_prefix(3);
			return { ScreenshotFilenameTemplateUnitType::Year, 0 };
		case 'y':
			str.remove_prefix(3);
			return { ScreenshotFilenameTemplateUnitType::Year2, 0 };
		case 'm':
			str.remove_prefix(3);
			return { ScreenshotFilenameTemplateUnitType::Month, 0 };
		case 'D':
			str.remove_prefix(3);
			return { ScreenshotFilenameTemplateUnitType::Day, 0 };
		case 'H':
			str.remove_prefix(3);
			return { ScreenshotFilenameTemplateUnitType::Hour, 0 };
		case 'M':
			str.remove_prefix(3);
			return { ScreenshotFilenameTemplateUnitType::Minite, 0 };
		case 'S':
			str.remove_prefix(3);
			return { ScreenshotFilenameTemplateUnitType::Second, 0 };
		default:
			str.remove_prefix(1);
			return { ScreenshotFilenameTemplateUnitType::Character, (int8_t)curChar };
		}
	}

	// 最大的字符数量限制
	constexpr uint8_t MAX_CHAR_COUNT_LIMIT = 99;

	// 检查 %WT[:nn]% 和 %PN[:nn]%
	if (str.size() >= 4 && ((nextChar == 'W' && str[2] == 'T') || (nextChar == 'P' && str[2] == 'N'))) {
		if (str[3] == '%') {
			str.remove_prefix(4);
			return { nextChar == 'W' ? ScreenshotFilenameTemplateUnitType::WindowTitle :
				ScreenshotFilenameTemplateUnitType::ProcessName, 0 };
		} else if (str[3] == ':') {
			uint8_t limit;
			auto result = std::from_chars(str.data() + 4, str.data() + str.size(), limit);

			if (result.ec == std::errc{} && limit <= MAX_CHAR_COUNT_LIMIT &&
				result.ptr != str.data() + str.size() && *result.ptr == '%')
			{
				str.remove_prefix(result.ptr - str.data() + 1);
				return { nextChar == 'W' ? ScreenshotFilenameTemplateUnitType::WindowTitle :
					ScreenshotFilenameTemplateUnitType::ProcessName, (int8_t)limit };
			}
		}
	}

	str.remove_prefix(1);
	return { ScreenshotFilenameTemplateUnitType::Character, (int8_t)curChar };
}

static bool IsValidUnit(ScreenshotFilenameTemplateUnit unit) noexcept {
	// 不允许无法作为文件名的特殊字符，禁止的字符列表来自
	// https://learn.microsoft.com/en-us/windows/win32/fileio/naming-a-file#naming-conventions
	if (unit.type == ScreenshotFilenameTemplateUnitType::Character) {
		constexpr std::string_view FORBIDDEN_LIST = "<>:\"/\\|?*";
		return FORBIDDEN_LIST.find(unit.value) == std::string_view::npos;
	} else {
		return true;
	}
}

bool ScreenshotFilenameTemplateParser::IsValid(std::string_view str) noexcept {
	// 如果 WT 和 PN 不使用冒号，这里可以直接检查字符串是否包含禁止的字符，
	// 但我找不到比冒号更合适的符号了。
	while (!str.empty()) {
		if (!IsValidUnit(GetNextUnit(str))) {
			return false;
		}
	}

	return true;
}

bool ScreenshotFilenameTemplateParser::Parse(
	std::string_view str,
	ScreenshotFilenameTemplate& result
) noexcept {
	assert(result.units.empty());

	while (!str.empty()) {
		ScreenshotFilenameTemplateUnit unit = GetNextUnit(str);

		if (!IsValidUnit(unit)) {
			result.units.clear();
			return false;
		}

		result.units.push_back(std::move(unit));
	}
	
	return true;
}

}
