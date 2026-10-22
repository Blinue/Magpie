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

	// 导出 png 时 pixelData 必须是 R16G16B16A16_UNROM 格式。WCG 下使用 BT.2020 SDR，
	// HDR 下使用 BT.2020/PQ。
	// pixelData 的最后一行应有填充位，这是 WIC 要求的。
	static HRESULT Save(
		wil::zwstring_view fileName,
		SizeU textureSize,
		EffectTextureFormat textureFormat,
		std::span<uint8_t> pixelData,
		uint32_t rowPitch,
		const ColorInfo& colorInfo
	) noexcept;
};

}
