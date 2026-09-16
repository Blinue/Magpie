#pragma once

namespace Magpie {

struct ColorHelper {
	static float SrgbToLinear(uint8_t c) noexcept {
		static std::array<float, 256> lut = [] {
			std::array<float, 256> result{};
			for (uint32_t i = 0; i < 256; ++i) {
				float c = i / 255.0f;
				if (c <= 0.04045f) {
					result[i] = c / 12.92f * 255.0f;
				} else {
					result[i] = std::pow((c + 0.055f) / 1.055f, 2.4f) * 255.0f;
				}
			}
			return result;
		}();

		return lut[c];
	}
};

}
