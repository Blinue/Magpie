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

	// 不支持在运行时更改语言，因此我们可以缓存字体数据
	std::vector<BYTE> _buffer;
};

}
