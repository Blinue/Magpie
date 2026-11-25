#pragma once
#include "SmallVector.h"

namespace Magpie {

struct Extension {
	// 在 AppSettings 中的索引
	uint32_t idx = 0;

	std::string name;
	std::string author;
	std::string homePageUrl;
	std::string description;

	SmallVector<std::string, 0> imageEncoderFullIds;
	SmallVector<std::string, 0> effectFullIds;

	bool hasDll = false;
	bool hasError = false;
};

}
