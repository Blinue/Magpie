#pragma once
#include "pch.h"


class ImageReizer {
public:
	enum class FilterType {
		Nearest
	};

	static void Run(const BYTE* src, SIZE srcSize, BYTE* dest, SIZE destSize, FilterType filter);
};

