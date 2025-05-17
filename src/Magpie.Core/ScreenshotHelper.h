#pragma once

namespace Magpie {

struct ScreenshotHelper {
	// 失败则返回 0
	static uint32_t FindUnusedScreenshotNum(const std::filesystem::path& screenshotsDir) noexcept;

	static bool SavePng(
		uint32_t width,
		uint32_t height,
		std::span<uint8_t> pixelData,
		uint32_t rowPitch,
		const wchar_t* fileName
	) noexcept;
};

}
