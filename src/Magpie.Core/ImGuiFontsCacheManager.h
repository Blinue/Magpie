#pragma once
#include "Singleton.h"
#include <imgui.h>
#include <parallel_hashmap/phmap.h>

namespace Magpie {

class ImGuiFontsCacheManager : public Singleton<ImGuiFontsCacheManager> {
	friend Singleton<ImGuiFontsCacheManager>;

public:
	bool Load(std::wstring_view language, uint32_t dpi, ImFontAtlas& fontAltas) noexcept;

	void Save(std::wstring_view language, uint32_t dpi, const ImFontAtlas& fontAltas) noexcept;

private:
	ImGuiFontsCacheManager() = default;

	// dpi -> 字体数据
	phmap::flat_hash_map<uint32_t, std::vector<uint8_t>> _cacheMap;
};

}
