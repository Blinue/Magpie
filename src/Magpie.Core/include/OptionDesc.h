#pragma once
#include "SmallVector.h"
#include <variant>

namespace Magpie {

struct BoolOptionDesc {
	bool defaultValue;
};

struct IntOptionDesc {
	int defaultValue;
	int minValue;
	int maxValue;
	int step;
};

struct FloatOptionDesc {
	float defaultValue;
	float minValue;
	float maxValue;
	float step;
};

struct StringOptionDesc {
	std::string defaultValue;
};

struct SelectionOptionDesc {
	SmallVector<std::string, 0> items;
	uint32_t defaultValue;
};

using OptionValue = std::variant<bool, int, float, std::string, uint32_t>;

}
