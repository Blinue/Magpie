#pragma once
#include "pch.h"


namespace Magpie::Core {

class TextureLoader {
public:
	static winrt::com_ptr<ID3D11Texture2D> Load(const wchar_t* fileName);
};

}
