#pragma once
#include "SmallVector.h"

namespace Magpie {

struct CursorHelper {
	// 如果没有完美匹配则倾向于提取较大的资源
	static wil::unique_hcursor ExtractCursorFromModule(
		HMODULE hModule,
		LPCWSTR resName,
		uint32_t preferredWidth
	) noexcept;

	// 支持 .ico 和 .ani
	static bool ExtractCursorFramesFromFile(
		const wchar_t* fileName,
		uint32_t preferredWidth,
		SmallVectorImpl<wil::unique_hcursor>& frames,
		SmallVectorImpl<std::pair<uint32_t, std::chrono::nanoseconds>>& frameSequence
	) noexcept;

	// frames 中的句柄无需销毁
	static void TryResolveAnimatedCursor(
		HCURSOR hCursor,
		SmallVectorImpl<HCURSOR>& frames,
		SmallVectorImpl<std::pair<uint32_t, std::chrono::nanoseconds>>& frameSequence
	) noexcept;
};

}
