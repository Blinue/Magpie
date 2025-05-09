#pragma once

namespace Magpie {

struct ScreenshotHelper {
	static bool SavePng(
		uint32_t width,
		uint32_t height,
		std::span<uint8_t> pixelData,
		uint32_t rowPitch,
		const wchar_t* fileName
	) noexcept;
};

}
