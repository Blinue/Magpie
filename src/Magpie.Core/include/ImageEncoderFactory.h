#pragma once
#include "Singleton.h"
#include "ColorInfo.h"
#include "ImageEncoderMetadata.h"
#include <parallel_hashmap/phmap.h>

namespace Magpie {

class ImageEncoderFactory : public Singleton<ImageEncoderFactory> {
	friend Singleton<ImageEncoderFactory>;

public:
	using EncodeFn = HRESULT(
		const phmap::flat_hash_map<std::string, OptionValue>& optionValues,
		wil::zstring_view fileName,
		SizeU imageSize,
		std::span<const uint8_t> pixelData,
		uint32_t rowPitch,
		const ColorInfo& colorInfo
	) noexcept;

	void Initialize() noexcept;

	// id 为 {插件 id}/{编码器 id}，内置的编码器使用 "*" 作为插件 id。插件 id 必须
	// 合法的文件名，因此不能包含 / 和 *。 
	void RegisterImageEncoder(
		std::string_view id,
		ImageEncoderMetadata&& metadata,
		EncodeFn* encodeFn
	) noexcept;

	const ImageEncoderMetadata* GetEncoderMetadata(std::string_view id) const noexcept;;

	HRESULT EncodeImage(
		std::string_view id,
		const phmap::flat_hash_map<std::string, OptionValue>& optionValues,
		wil::zstring_view fileName,
		SizeU imageSize,
		std::span<const uint8_t> pixelData,
		uint32_t rowPitch,
		const ColorInfo& colorInfo
	) noexcept;

private:
	ImageEncoderFactory() = default;

	phmap::flat_hash_map<std::string, std::pair<ImageEncoderMetadata, EncodeFn*>> _encoders;
};

}
