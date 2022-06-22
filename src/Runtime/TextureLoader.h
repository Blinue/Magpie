#pragma once
#include "pch.h"


class TextureLoader {
public:
	static winrt::com_ptr<ID3D11Texture2D> Load(const wchar_t* fileName);
};
