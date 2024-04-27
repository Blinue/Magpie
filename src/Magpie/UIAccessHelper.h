#pragma once

namespace Magpie {

struct UIAccessHelper {
	static bool MakeExeUIAccess(const wchar_t* exePath, uint32_t version) noexcept;
	static bool ClearUIAccess() noexcept;
};

}
