#pragma once
#include "pch.h"
#include "App.h"


class TextureLoader {
public:
	static ComPtr<ID3D11Texture2D> Load(const wchar_t* fileName);
};

