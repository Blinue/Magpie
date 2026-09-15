#pragma once

namespace Magpie {

struct ScreenshotFilenameTemplateHelper {
	// 空字符串视为合法
	static bool IsValid(std::string_view templateStr) noexcept;

	static bool Apply(std::string_view templateStr, HWND hwndSrc, std::string& result) noexcept;
};

}
