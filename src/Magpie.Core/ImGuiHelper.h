#pragma once
#include <imgui.h>

namespace Magpie {

struct ImGuiHelper {
	static const ImWchar* GetGlyphRangesChineseSimplifiedOfficial() noexcept;
	static const ImWchar* GetGlyphRangesChineseTraditionalOfficial() noexcept;

	// Basic Latin
	static constexpr ImWchar BASIC_LATIN_RANGES[] = { 0x20, 0x7E };
	// Basic Latin + Latin-1 Supplement + Latin Extended-A，用于土耳其语、匈牙利语等。
	// 参见 https://en.wikipedia.org/wiki/Latin_Extended-A
	static constexpr ImWchar EXTENDED_LATIN_RANGES[] = { 0x20, 0x17F };
	// Basic Latin + Georgian + Georgian Supplement + Georgian Extended，用于格鲁吉亚语。
	// https://en.wikipedia.org/wiki/Georgian_scripts
	static constexpr ImWchar GEORGIAN_RANGES[] = { 0x20, 0x7E, 0x10A0, 0x10FF, 0x2D00, 0x2D2F, 0x1C90, 0x1CBF };
	// 不包含 Basic Latin，用于 Win11
	static constexpr ImWchar GEORGIAN_EXTRA_RANGES[] = { 0x10A0, 0x10FF, 0x2D00, 0x2D2F, 0x1C90, 0x1CBF };
	// Tamil，用于泰米尔语。Tamil Supplement 超出了 ImWchar16 的存储范围，因此暂不支持。
	// 参见 https://en.wikipedia.org/wiki/Tamil_script
	static constexpr ImWchar TAMIL_EXTRA_RANGES[] = { 0xB80, 0xBFF };
};

}
