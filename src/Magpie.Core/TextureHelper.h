#pragma once
#include "EffectDesc.h"

namespace Magpie {

class TextureHelper {
public:
	// 支持 dds、bmp、jpg、png 和 tiff
	static winrt::com_ptr<ID3D11Texture2D> LoadTexture(const wchar_t* fileName, ID3D11Device* d3dDevice) noexcept;

	// 支持 dds 和 png
	static bool SaveTexture(
		const wchar_t* fileName,
		uint32_t width,
		uint32_t height,
		EffectIntermediateTextureFormat format,
		std::span<uint8_t> pixelData,
		uint32_t rowPitch
	) noexcept;
};

}
