#pragma once
#include <imgui.h>

namespace Magpie::Core {

struct ImGuiHelper {
	static const ImWchar* GetGlyphRangesChineseSimplifiedOfficial() noexcept;
	static const ImWchar* GetGlyphRangesChineseTraditionalOfficial() noexcept;

	static constexpr ImWchar NUMBER_RANGES[] = { L'0', L'9', 0 };
	static constexpr ImWchar NOT_NUMBER_RANGES[] = { 0x20, L'0' - 1, L'9' + 1, 0x7E, 0 };
	// Basic Latin
	static constexpr ImWchar ENGLISH_RANGES[] = { 0x20, 0x7E, 0 };
	// Basic Latin + Latin-1 Supplement + Latin Extended-A，用于土耳其语、匈牙利语等
	// 参见 https://en.wikipedia.org/wiki/Latin_Extended-A
	static constexpr ImWchar Latin_1_Extended_A_RANGES[] = { 0x20, 0x17F, 0 };
};

}
