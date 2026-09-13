#pragma once

namespace Magpie {

struct ScreenshotFilenameHelper {
	// 空字符串视为合法
	static bool IsValidTemplate(std::string_view templateStr) noexcept;

	static bool GenerateFilename(
		std::string_view templateStr,
		const std::filesystem::path& screenshotDir,
		HWND hwndSrc
	) noexcept;
};

}
