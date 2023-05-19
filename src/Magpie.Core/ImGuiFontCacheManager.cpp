#include "pch.h"
#include "ImGuiFontCacheManager.h"
#include <imgui.h>

namespace Magpie::Core {

void ImGuiFontCacheManager::Save(std::wstring_view language, const ImFontAtlas& fontAltas) noexcept {

}

bool ImGuiFontCacheManager::Load(std::wstring_view language, ImFontAtlas& fontAltas) noexcept {
	return false;
}

}
