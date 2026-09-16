#pragma once

namespace Magpie {

struct ScreenshotFilenameTemplateHelper {
	static bool IsValid(std::string_view templateStr) noexcept;

	static bool Apply(std::string_view templateStr, HWND hwndSrc, std::string& result) noexcept;
};

}
