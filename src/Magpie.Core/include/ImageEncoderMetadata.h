#pragma once
#include "SmallVector.h"
#include "OptionDesc.h"
#include <variant>

namespace Magpie {

struct ImageEncoderOptionDesc {
	std::string id;
	std::string displayName;
	std::string description;

	std::variant<StringOptionDesc, BoolOptionDesc, IntOptionDesc, FloatOptionDesc> valueOption;
};

struct ImageEncoderMetadata {
	std::string displayName;
	std::string description;
	std::string fileExtension;

	SmallVector<ImageEncoderOptionDesc, 0> optionDescs;
};

}
