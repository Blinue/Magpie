#include "pch.h"
#include "StrUtils.h"


void StrUtils::Trim(std::string_view& str) {
	for (int i = 0; i < str.size(); ++i) {
		if (!isspace(str[i])) {
			str.remove_prefix(i);

			size_t i = str.size() - 1;
			for (; i > 0; --i) {
				if (!isspace(str[i])) {
					break;
				}
			}
			str.remove_suffix(str.size() - 1 - i);
			return;
		}
	}

	str.remove_prefix(str.size());
}
