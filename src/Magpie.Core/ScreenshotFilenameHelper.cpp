#include "pch.h"
#include "ScreenshotFilenameHelper.h"
#include "AppXReader.h"
#include "SmallVector.h"
#include "StrHelper.h"
#include "Win32Helper.h"
#include <fmt/chrono.h>

namespace Magpie {

enum class TemplateTokenType : uint8_t {
	Character,
	// %WT:nn%: 可限制字符数量，最大是 99
	WindowTitle,
	// %PN:nn%: 可限制字符数量，最大是 99
	ProcessName,
	// %Y%
	Year,
	// %y%: 年份的最后两位
	ShortYear,
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

struct TemplateToken {
	TemplateTokenType type = TemplateTokenType::Character;
	// Character: 表示 UTF-8 字符码
	// WindowTitle 和 ProcessName: 表示最大字符数量，非正值为无限制
	// 其他类型不使用
	int8_t value = 0;
};

static TemplateToken GetNextToken(std::string_view& str) noexcept {
	assert(!str.empty());

	const char curChar = str[0];
	
	if (curChar != '%' || str.size() < 3) {
		str.remove_prefix(1);
		return { TemplateTokenType::Character, (int8_t)curChar };
	}

	const char nextChar = str[1];

	if (str[2] == '%') {
		switch (nextChar) {
		case 'Y':
			str.remove_prefix(3);
			return { TemplateTokenType::Year, 0 };
		case 'y':
			str.remove_prefix(3);
			return { TemplateTokenType::ShortYear, 0 };
		case 'm':
			str.remove_prefix(3);
			return { TemplateTokenType::Month, 0 };
		case 'D':
			str.remove_prefix(3);
			return { TemplateTokenType::Day, 0 };
		case 'H':
			str.remove_prefix(3);
			return { TemplateTokenType::Hour, 0 };
		case 'M':
			str.remove_prefix(3);
			return { TemplateTokenType::Minite, 0 };
		case 'S':
			str.remove_prefix(3);
			return { TemplateTokenType::Second, 0 };
		default:
			str.remove_prefix(1);
			return { TemplateTokenType::Character, (int8_t)curChar };
		}
	}

	// 最大的字符数量限制
	constexpr uint8_t MAX_CHAR_COUNT_LIMIT = 99;

	// 检查 %WT[:nn]% 和 %PN[:nn]%
	if (str.size() >= 4 && ((nextChar == 'W' && str[2] == 'T') || (nextChar == 'P' && str[2] == 'N'))) {
		if (str[3] == '%') {
			str.remove_prefix(4);
			return { nextChar == 'W' ? TemplateTokenType::WindowTitle :
				TemplateTokenType::ProcessName, 0 };
		} else if (str[3] == ':') {
			uint8_t limit;
			auto result = std::from_chars(str.data() + 4, str.data() + str.size(), limit);

			if (result.ec == std::errc{} && limit > 0 && limit <= MAX_CHAR_COUNT_LIMIT &&
				result.ptr != str.data() + str.size() && *result.ptr == '%')
			{
				str.remove_prefix(result.ptr - str.data() + 1);
				return { nextChar == 'W' ? TemplateTokenType::WindowTitle :
					TemplateTokenType::ProcessName, (int8_t)limit };
			}
		}
	}

	str.remove_prefix(1);
	return { TemplateTokenType::Character, (int8_t)curChar };
}

static bool IsValidUnit(TemplateToken token) noexcept {
	// 不允许无法作为文件名的特殊字符，禁止的字符列表来自
	// https://learn.microsoft.com/en-us/windows/win32/fileio/naming-a-file#naming-conventions
	if (token.type == TemplateTokenType::Character) {
		constexpr std::string_view FORBIDDEN_LIST = "<>:\"/\\|?*";
		return FORBIDDEN_LIST.find(token.value) == std::string_view::npos;
	} else {
		return true;
	}
}

bool ScreenshotFilenameHelper::IsValidTemplate(std::string_view templateStr) noexcept {
	// 如果 WT 和 PN 不使用冒号，这里可以直接检查字符串是否包含禁止的字符，
	// 但我找不到比冒号更合适的符号了。
	while (!templateStr.empty()) {
		if (!IsValidUnit(GetNextToken(templateStr))) {
			return false;
		}
	}

	return true;
}

static std::string_view ExtractUTF8CodePoints(const std::string& str, int8_t codePointCount) noexcept {
	assert(!str.empty());

	if (codePointCount <= 0) {
		// 无字符数量限制
		return str;
	}

	int8_t curCodePointCount = 0;

	for (size_t i = 0; i < str.size();) {
		const uint8_t byte = (uint8_t)str[i];

		// UTF-8 序列结构见 https://en.wikipedia.org/wiki/UTF-8
		// 0xxxxxxx - ASCII 字符
		// 11yyyxxx - 多字节字符的首字节
		// 10xxxxxx - 多字节字符的后续字节
		if ((byte & (uint8_t)0b11000000) == (uint8_t)0b11000000) {
			// 多字节字符的首字节，应跳过后续字节
			if ((byte & (uint8_t)0b11100000) == (uint8_t)0b11000000) {
				i += 2;
			} else if ((byte & (uint8_t)0b11110000) == (uint8_t)0b11100000) {
				i += 3;
			} else if ((byte & (uint8_t)0b11111000) == (uint8_t)0b11110000) {
				i += 4;
			} else {
				// 跳过无效字节
				++i;
				continue;
			}

			if (i >= str.size()) {
				break;
			}
		} else {
			++i;

			// 不是 ASCII 字符则是无效字节
			if ((byte & (uint8_t)0b10000000) != 0) {
				continue;
			}
		}

		if (++curCodePointCount == codePointCount) {
			return std::string_view(str.data(), i);
		}
	}

	// 返回整个字符串
	return str;
}

bool ScreenshotFilenameHelper::GenerateFilename(
	std::string_view templateStr,
	const std::filesystem::path& /*screenshotDir*/,
	HWND hwndSrc
) noexcept {
	SmallVector<TemplateToken> tokens;

	bool hasWindowTitleToken = false;
	bool hasProcessNameToken = false;
	bool hasDateToken = false;

	while (!templateStr.empty()) {
		TemplateToken token = GetNextToken(templateStr);

		if (!IsValidUnit(token)) {
			return false;
		}

		if (token.type == TemplateTokenType::WindowTitle) {
			hasWindowTitleToken = true;
		} else if (token.type == TemplateTokenType::ProcessName) {
			hasProcessNameToken = true;
		} else if (token.type != TemplateTokenType::Character) {
			hasDateToken = true;
		}

		tokens.push_back(std::move(token));
	}

	std::string windowTitle;
	std::string processName;
	std::string year;
	std::string month;
	std::string day;
	std::string hour;
	std::string minite;
	std::string second;

	if (hasWindowTitleToken) {
		windowTitle = StrHelper::UTF16ToUTF8(Win32Helper::GetWindowTitle(hwndSrc));

		// 失败或标题为空时使用默认值
		if (windowTitle.empty()) {
			windowTitle = "[Empty]";
		}
	}

	if (hasProcessNameToken) {
		// 打包应用使用应用名
		AppXReader appxReader;
		processName = StrHelper::UTF16ToUTF8(appxReader.Initialize(hwndSrc) ?
			appxReader.GetDisplayName() : Win32Helper::GetProcessDescriptionFromWindow(hwndSrc));

		// 失败时回落到可执行文件名
		if (processName.empty()) {
			processName = StrHelper::UTF16ToUTF8(Win32Helper::GetWindowExeName(hwndSrc));

			if (processName.empty()) {
				// 最后的默认值，一般不会执行到这里
				processName = "[Unknown]";
			} else if (processName.ends_with(".exe")) {
				// 删除扩展名
				processName.erase(processName.size() - 4);
			}
		}
	}

	if (hasDateToken) {
		// 不使用 fmt，它似乎不支持时区
		SYSTEMTIME localTime;
		GetLocalTime(&localTime);

		year = StrHelper::ToString(localTime.wYear);
		assert(year.size() == 4);

		month = StrHelper::ToString(localTime.wMonth);
		if (month.size() == 1) {
			month.insert(month.begin(), '0');
		}

		day = StrHelper::ToString(localTime.wDay);
		if (day.size() == 1) {
			day.insert(day.begin(), '0');
		}

		hour = StrHelper::ToString(localTime.wHour);
		if (hour.size() == 1) {
			hour.insert(hour.begin(), '0');
		}

		minite = StrHelper::ToString(localTime.wMinute);
		if (minite.size() == 1) {
			minite.insert(minite.begin(), '0');
		}

		second = StrHelper::ToString(localTime.wSecond);
		if (second.size() == 1) {
			second.insert(second.begin(), '0');
		}
	}

	std::string fileName;

	for (TemplateToken token : tokens) {
		switch (token.type) {
		case TemplateTokenType::Character:
			fileName.push_back(token.value);
			break;
		case TemplateTokenType::WindowTitle:
			fileName.insert(fileName.size(), ExtractUTF8CodePoints(windowTitle, token.value));
			break;
		case TemplateTokenType::ProcessName:
			fileName.insert(fileName.size(), ExtractUTF8CodePoints(processName, token.value));
			break;
		case TemplateTokenType::Year:
			fileName.insert(fileName.size(), year);
			break;
		case TemplateTokenType::ShortYear:
			fileName.insert(fileName.size(), std::string_view(year.data() + 2, 2));
			break;
		case TemplateTokenType::Month:
			fileName.insert(fileName.size(), month);
			break;
		case TemplateTokenType::Day:
			fileName.insert(fileName.size(), day);
			break;
		case TemplateTokenType::Hour:
			fileName.insert(fileName.size(), hour);
			break;
		case TemplateTokenType::Minite:
			fileName.insert(fileName.size(), minite);
			break;
		default:
			assert(token.type == TemplateTokenType::Second);
			fileName.insert(fileName.size(), second);
			break;
		}
	}
	
	return false;
}

}
