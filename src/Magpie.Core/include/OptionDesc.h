#pragma once
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

using OptionValue = std::variant<bool, int, float, std::string>;

}
