#pragma once

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
};

}
