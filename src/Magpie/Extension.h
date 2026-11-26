#pragma once
#include "SmallVector.h"
#include "Version.h"

namespace Magpie {

struct Extension {
	// 在 AppSettings 中的索引
	uint32_t idx = 0;

	std::string name;
	std::string description;
	// 未提供版本号则为 0.0.0
	Version version;
	std::string author;
	winrt::Uri homePage{ nullptr };
	
	SmallVector<std::string, 0> imageEncoderFullIds;
	SmallVector<std::string, 0> effectFullIds;

	bool hasDll = false;
	bool hasError = false;
};

}
