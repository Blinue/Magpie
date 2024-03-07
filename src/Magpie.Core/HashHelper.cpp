#include "pch.h"
#include "HashHelper.h"
#include "Utils.h"

namespace Magpie::Core {

std::wstring HashHelper::HexHash(std::span<const uint8_t> data) noexcept {
	uint64_t hashBytes = Utils::HashData(data);

	static wchar_t oct2Hex[16] = {
		L'0',L'1',L'2',L'3',L'4',L'5',L'6',L'7',
		L'8',L'9',L'a',L'b',L'c',L'd',L'e',L'f'
	};

	std::wstring result(16, 0);
	wchar_t* pResult = &result[0];

	BYTE* b = (BYTE*)&hashBytes;
	for (int i = 0; i < 8; ++i) {
		*pResult++ = oct2Hex[(*b >> 4) & 0xf];
		*pResult++ = oct2Hex[*b & 0xf];
		++b;
	}

	return result;
}

}
