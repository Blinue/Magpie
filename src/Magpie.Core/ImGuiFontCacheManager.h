#pragma once

struct ImFontAtlas;

namespace Magpie::Core {

class ImGuiFontCacheManager {
public:
	static ImGuiFontCacheManager& Get() noexcept {
		static ImGuiFontCacheManager instance;
		return instance;
	}

	ImGuiFontCacheManager(const ImGuiFontCacheManager&) = delete;
	ImGuiFontCacheManager(ImGuiFontCacheManager&&) = delete;

	bool Load(std::wstring_view language, ImFontAtlas& fontAltas) noexcept;

	void Save(std::wstring_view language, const ImFontAtlas& fontAltas) noexcept;

private:
	ImGuiFontCacheManager() = default;
};

}
