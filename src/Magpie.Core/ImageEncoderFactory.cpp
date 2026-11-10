#include "pch.h"
#include "ImageEncoderFactory.h"
#include "PngImageEncoder.h"

namespace Magpie {

void ImageEncoderFactory::Initialize() noexcept {
	RegisterImageEncoder(
		"*/png",
		PngImageEncoder::GetMetadata(),
		&PngImageEncoder::Encode
	);
}

void ImageEncoderFactory::RegisterImageEncoder(
	std::string_view id,
	ImageEncoderMetadata&& metadata,
	EncodeFn* encodeFn
) noexcept {
	assert(!_encoders.contains(id));
	_encoders.emplace(id, std::make_pair(std::move(metadata), encodeFn));
}

const ImageEncoderMetadata* ImageEncoderFactory::GetEncoderMetadata(std::string_view id) const noexcept {
	if (auto it = _encoders.find(id); it != _encoders.end()) {
		return &it->second.first;
	} else {
		return nullptr;
	}
}

HRESULT ImageEncoderFactory::EncodeImage(
	std::string_view id,
	const phmap::flat_hash_map<std::string, OptionValue>& optionValues,
	wil::zstring_view fileName,
	SizeU imageSize,
	std::span<const uint8_t> pixelData,
	uint32_t rowPitch,
	const ColorInfo& colorInfo
) noexcept {
	if (auto it = _encoders.find(id); it != _encoders.end()) {
		return it->second.second(
			optionValues, fileName, imageSize, pixelData, rowPitch, colorInfo);
	} else {
		return E_NOTIMPL;
	}
}

}
