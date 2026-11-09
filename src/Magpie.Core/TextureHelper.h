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

	// pixelData 必须是 R16G16B16_UNROM 格式。WCG 下使用 BT.2020 SDR，HDR 下
	// 使用 BT.2020/PQ。
	static HRESULT SavePng(
		wil::zwstring_view fileName,
		SizeU imgSize,
		std::span<uint8_t> pixelData,
		uint32_t rowPitch,
		const ColorInfo& colorInfo
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
