#include "pch.h"
#include "ImageEncoderFactory.h"
#include "PngImageEncoder.h"
#include "Logger.h"
#include "StrHelper.h"

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
	std::function<EncodeFn> encodeFn
) noexcept {
	assert(!_encoders.contains(id));
	_encoders.emplace(id, std::make_pair(std::move(metadata), std::move(encodeFn)));
	Logger::Get().Info(StrHelper::Concat("已注册图像编码器 ", id));
}

bool ImageEncoderFactory::UnregisterImageEncoder(std::string_view id) noexcept {
	size_t count = _encoders.erase(id);
	if (count > 0) {
		Logger::Get().Info(StrHelper::Concat("已取消注册图像编码器 ", id));
		return true;
	} else {
		return false;
	}
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
	winrt::AdvancedColorKind colorKind
) noexcept {
	if (auto it = _encoders.find(id); it != _encoders.end()) {
		return it->second.second(
			optionValues, fileName, imageSize, pixelData, rowPitch, colorKind);
	} else {
		return E_NOTIMPL;
	}
}

}
