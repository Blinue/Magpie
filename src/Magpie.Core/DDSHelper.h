#pragma once

namespace Magpie {

struct DDSHelper {
	static winrt::com_ptr<ID3D11Texture2D> Load(
		const wchar_t* fileName, ID3D11Device* d3dDevice) noexcept;

	static bool Save(
		const wchar_t* fileName,
		uint32_t width,
		uint32_t height,
		DXGI_FORMAT format,
		std::span<uint8_t> pixelData,
		uint32_t rowPitch
	);
};

}
