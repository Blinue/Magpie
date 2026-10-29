#pragma once
#include "ShaderEffectDrawInfo.h"

namespace Magpie {

class D3D12Context;

struct TextureHelper {
	// 支持 dds、bmp、jpg、png 和 tif。自动检测 sRGB 并修改 format。
	static winrt::com_ptr<ID3D12Resource> LoadFromFile(
		wil::zwstring_view fileName,
		const D3D12Context& d3d12Context,
		DXGI_FORMAT& format,
		SizeU& textureSize
	) noexcept;

	// 中间纹理导出为 dds，否则导出为 png。
	// dds 不使用 acKind。
	// 导出 png 时 pixelData 必须是 R16G16B16A16_UNROM 格式。WCG 下使用 BT.2020 色域，HDR 下使用 BT.2020
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
