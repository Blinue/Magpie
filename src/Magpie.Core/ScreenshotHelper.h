#pragma once

namespace Magpie {

struct ScreenshotHelper {
	// 失败则返回 0
	static uint32_t FindUnusedScreenshotNum(const std::filesystem::path& screenshotsDir) noexcept;
};

}
