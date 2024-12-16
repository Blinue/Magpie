#pragma once

namespace Magpie {

class TextureLoader {
public:
	static winrt::com_ptr<ID3D11Texture2D> Load(const wchar_t* fileName, ID3D11Device* d3dDevice) noexcept;
};

}
