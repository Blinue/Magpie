#include "pch.h"
#include "ScreenshotFilenameTemplateHelper.h"
#include "AppXReader.h"
#include "SmallVector.h"
#include "StrHelper.h"
#include "Win32Helper.h"

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
	Second,
	// %MS%
	Millisecond
};

struct TemplateToken {
	TemplateTokenType type;
	// Character: 表示 UTF-8 字节
	// WindowTitle 和 ProcessName: 表示最大字符数量，非正值为无限制
	// 其他类型不使用
	int8_t value;
};

// 不允许出现在文件名中的特殊字符，来自
// https://learn.microsoft.com/en-us/windows/win32/fileio/naming-a-file#naming-conventions
constexpr std::string_view FILENAME_FORBIDDEN_CHARS = "<>:\"/\\|?*";

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

	// 检查 %WT[:nn]%、%PN[:nn]% 和 %MS%
	if (str.size() >= 4) {
		if (nextChar == 'M') {
			if (str[2] == 'S' && str[3] == '%') {
				str.remove_prefix(4);
				return { TemplateTokenType::Millisecond, 0 };
			}
		} else if ((nextChar == 'W' && str[2] == 'T') || (nextChar == 'P' && str[2] == 'N')) {
			if (str[3] == '%') {
				str.remove_prefix(4);
				return { nextChar == 'W' ? TemplateTokenType::WindowTitle :
					TemplateTokenType::ProcessName, 0 };
			} else if (str[3] == ':') {
				// 最大的字符数量限制
				constexpr uint8_t MAX_CHAR_COUNT_LIMIT = 99;

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
	}

	str.remove_prefix(1);
	return { TemplateTokenType::Character, (int8_t)curChar };
}

static bool IsValidUnit(TemplateToken token) noexcept {
	if (token.type == TemplateTokenType::Character) {
		return FILENAME_FORBIDDEN_CHARS.find(token.value) == std::string_view::npos;
	} else {
		return true;
	}
}

bool ScreenshotFilenameTemplateHelper::IsValid(std::string_view templateStr) noexcept {
	// 如果 WT 和 PN 不使用冒号，这里可以直接检查字符串是否包含禁止的字符，
	// 但我找不到比冒号更合适的符号了。
	while (!templateStr.empty()) {
		if (!IsValidUnit(GetNextToken(templateStr))) {
			return false;
		}
	}

	return true;
}

// 替换特殊字符使得可以作为文件名
static void MakeValidFilename(std::string& str) noexcept {
	for (char& c : str) {
		if (FILENAME_FORBIDDEN_CHARS.find(c) != std::string_view::npos) {
			c = '#';
		}
	}
}

// 假设 str 是合法的 UTF-8 序列
static std::string_view ExtractUTF8Chars(const std::string& str, int8_t codePointCount) noexcept {
	assert(!str.empty());

	// codePointCount 非正表示无字符数量限制
	if (codePointCount <= 0) {
		return str;
	}

	int8_t curCodePointCount = 0;

	size_t i = 0;
	while (true) {
		const uint8_t byte = (uint8_t)str[i++];

		// UTF-8 序列结构见 https://en.wikipedia.org/wiki/UTF-8
		// 0xxxxxxx - ASCII 字符
		// 11yyyxxx - 多字节字符的首字节
		// 10xxxxxx - 多字节字符的后续字节
		if ((byte & (uint8_t)0b11000000) == (uint8_t)0b11000000) {
			// 多字节字符的首字节，应跳过后续字节
			if ((byte & (uint8_t)0b11100000) == (uint8_t)0b11000000) {
				i += 1;
			} else if ((byte & (uint8_t)0b11110000) == (uint8_t)0b11100000) {
				i += 2;
			} else {
				assert((byte & (uint8_t)0b11111000) == (uint8_t)0b11110000);
				i += 3;
			}
		} else {
			// 必是 ASCII 字符
			assert((byte & (uint8_t)0b10000000) == 0);
		}

		if (i >= str.size()) {
			// 返回整个字符串
			return str;
		}

		if (++curCodePointCount == codePointCount) {
			return std::string_view(str.data(), i);
		}
	}
}

bool ScreenshotFilenameTemplateHelper::Apply(
	std::string_view templateStr,
	HWND hwndSrc,
	std::string& result
) noexcept {
	assert(!templateStr.empty());

	SmallVector<TemplateToken> tokens;

	bool hasWindowTitleToken = false;
	bool hasProcessNameToken = false;
	bool hasDateToken = false;

	do {
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
	} while (!templateStr.empty());

	std::string windowTitle;
	std::string processName;
	std::string year;
	std::string_view shortYear;
	std::string month;
	std::string day;
	std::string hour;
	std::string minite;
	std::string second;
	std::string milliseconds;

	if (hasWindowTitleToken) {
		windowTitle = StrHelper::UTF16ToUTF8(Win32Helper::GetWindowTitle(hwndSrc));
		StrHelper::Trim(windowTitle);
		MakeValidFilename(windowTitle);
	}

	if (hasProcessNameToken) {
		// 打包应用使用应用名
		AppXReader appxReader;
		processName = StrHelper::UTF16ToUTF8(appxReader.Initialize(hwndSrc) ?
			appxReader.GetDisplayName() : Win32Helper::GetProcessDescriptionFromWindow(hwndSrc));
		StrHelper::Trim(processName);
		
		// 失败时回落到可执行文件名
		if (processName.empty()) {
			processName = StrHelper::UTF16ToUTF8(Win32Helper::GetWindowExeName(hwndSrc));
			
			// 删除扩展名
			if (processName.ends_with(".exe")) {
				processName.erase(processName.size() - 4);
			}
		}

		MakeValidFilename(processName);
	}

	if (hasDateToken) {
		// 不使用 fmt，它似乎不支持时区
		SYSTEMTIME localTime;
		GetLocalTime(&localTime);

		year = StrHelper::ToString(localTime.wYear);
		
		if (year.size() == 4) {
			shortYear = std::string_view(year.data() + 2, 2);
		} else {
			assert(false);
			shortYear = year;
		}

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

		milliseconds = StrHelper::ToString(localTime.wMilliseconds);
		if (milliseconds.size() < 3) {
			milliseconds.insert(0, 3 - milliseconds.size(), '0');
		}
	}

	for (TemplateToken token : tokens) {
		switch (token.type) {
		case TemplateTokenType::Character:
			result.push_back(token.value);
			break;
		case TemplateTokenType::WindowTitle:
			result.insert(result.size(),
				windowTitle.empty() ? "[Empty]" : ExtractUTF8Chars(windowTitle, token.value));
			break;
		case TemplateTokenType::ProcessName:
			result.insert(result.size(),
				processName.empty() ? "[Unknown]" : ExtractUTF8Chars(processName, token.value));
			break;
		case TemplateTokenType::Year:
			result.insert(result.size(), year);
			break;
		case TemplateTokenType::ShortYear:
			result.insert(result.size(), shortYear);
			break;
		case TemplateTokenType::Month:
			result.insert(result.size(), month);
			break;
		case TemplateTokenType::Day:
			result.insert(result.size(), day);
			break;
		case TemplateTokenType::Hour:
			result.insert(result.size(), hour);
			break;
		case TemplateTokenType::Minite:
			result.insert(result.size(), minite);
			break;
		case TemplateTokenType::Second:
			result.insert(result.size(), second);
			break;
		default:
			assert(token.type == TemplateTokenType::Millisecond);
			result.insert(result.size(), milliseconds);
			break;
		}
	}

	assert(!result.empty());
	return true;
}

}
