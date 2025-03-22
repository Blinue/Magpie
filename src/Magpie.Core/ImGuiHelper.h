#pragma once
#include <imgui.h>

namespace Magpie {

struct ImGuiHelper {
	/////////////////////////////////////////////////////
	// 
	// 下面用于默认字体，不以 0 结尾
	// 
	/////////////////////////////////////////////////////

	// Basic Latin
	static constexpr ImWchar BASIC_LATIN_RANGES[] = { 0x20, 0x7E };
	// Basic Latin + Latin-1 Supplement + Latin Extended-A，用于土耳其语、匈牙利语等。
	// 参见 https://en.wikipedia.org/wiki/Latin_Extended-A
	static constexpr ImWchar EXTENDED_LATIN_RANGES[] = { 0x20, 0x17F };
	// Basic Latin + Georgian + Georgian Supplement + Georgian Extended，用于格鲁吉亚语。
	// https://en.wikipedia.org/wiki/Georgian_scripts
	static constexpr ImWchar GEORGIAN_RANGES[] = { 0x20, 0x7E, 0x10A0, 0x10FF, 0x2D00, 0x2D2F, 0x1C90, 0x1CBF };

	/////////////////////////////////////////////////////
	// 
	// 下面用于额外加载的字体，不包含 Basic Latin
	// 
	/////////////////////////////////////////////////////
	
	// 用于 Win11
	static constexpr ImWchar EXTRA_GEORGIAN_RANGES[] = { 0x10A0, 0x10FF, 0x2D00, 0x2D2F, 0x1C90, 0x1CBF, 0 };
	// Tamil，用于泰米尔语。Tamil Supplement 超出了 ImWchar16 的存储范围，因此暂不支持。
	// 参见 https://en.wikipedia.org/wiki/Tamil_script
	static constexpr ImWchar EXTRA_TAMIL_RANGES[] = { 0xB80, 0xBFF, 0 };

	static const ImWchar* GetGlyphRangesChineseSimplifiedOfficial() noexcept;
	static const ImWchar* GetGlyphRangesChineseTraditionalOfficial() noexcept;

	/////////////////////////////////////////////////////
	// 
	// 下面用于等宽字体
	// 
	/////////////////////////////////////////////////////
	
	static constexpr ImWchar NUMBER_RANGES[] = { L'0', L'9', 0 };
	static constexpr ImWchar NOT_NUMBER_RANGES[] = { BASIC_LATIN_RANGES[0], L'0' - 1, L'9' + 1, BASIC_LATIN_RANGES[1], 0};
};

}
