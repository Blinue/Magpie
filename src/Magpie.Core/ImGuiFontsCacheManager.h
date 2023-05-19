#pragma once

struct ImFontAtlas;

namespace Magpie::Core {

class ImGuiFontsCacheManager {
public:
	static ImGuiFontsCacheManager& Get() noexcept {
		static ImGuiFontsCacheManager instance;
		return instance;
	}

	ImGuiFontsCacheManager(const ImGuiFontsCacheManager&) = delete;
	ImGuiFontsCacheManager(ImGuiFontsCacheManager&&) = delete;

	bool Load(std::wstring_view language, ImFontAtlas& fontAltas) noexcept;

	void Save(std::wstring_view language, const ImFontAtlas& fontAltas) noexcept;

private:
	ImGuiFontsCacheManager() = default;
};

}
