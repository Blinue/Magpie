#pragma once
#include "EffectTextureFormat.h"
#include <D3D12MemAlloc.h>

namespace Magpie {

class GpuResourceManager;

struct TextureHelper {
	// 支持 dds、bmp、jpg、png 和 tif。自动检测 sRGB 并修改 format。
	static HRESULT LoadFromFile(
		wil::zwstring_view fileName,
		const GpuResourceManager& gpuResourceManager,
		DXGI_FORMAT& format,
		SizeU& textureSize,
		D3D12MA::Allocation** allocation
	) noexcept;

	static HRESULT SaveDDS(
		wil::zwstring_view fileName,
		SizeU textureSize,
		EffectTextureFormat textureFormat,
		std::span<uint8_t> pixelData,
		uint32_t rowPitch
	) noexcept;
};

}
