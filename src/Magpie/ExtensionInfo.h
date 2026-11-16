#pragma once
#include "SmallVector.h"

namespace Magpie {

struct ExtensionInfo {
	std::string extId;

	std::string displayName;
	std::string author;
	std::string homePageUrl;
	std::string description;

	SmallVector<std::string, 0> imageEncoderIds;
	SmallVector<std::string, 0> effectIds;
};

}
