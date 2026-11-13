#pragma once
#include "ColorInfo.h"
#include "ImageEncoderMetadata.h"
#include <parallel_hashmap/phmap.h>

namespace Magpie {

class PngImageEncoder {
public:
	static ImageEncoderMetadata GetMetadata() noexcept;;

	static HRESULT Encode(
		const phmap::flat_hash_map<std::string, OptionValue>& optionValues,
		wil::zstring_view fileName,
		SizeU imageSize,
		std::span<const uint8_t> pixelData,
		uint32_t rowPitch,
		winrt::AdvancedColorKind colorKind
	) noexcept;
};

}
