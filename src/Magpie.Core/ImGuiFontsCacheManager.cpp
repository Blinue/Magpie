#include "pch.h"
#include "ImGuiFontsCacheManager.h"
#include <imgui.h>
#include "YasHelper.h"
#include "Logger.h"
#include "Win32Utils.h"
#include "CommonSharedConstants.h"
#include "StrUtils.h"

namespace yas::detail {

// ImVector
template<std::size_t F, typename T>
struct serializer<
	type_prop::not_a_fundamental,
	ser_case::use_internal_serializer,
	F,
	ImVector<T>
> {
	template<typename Archive>
	static Archive& save(Archive& ar, const ImVector<T>& vector) noexcept {
		uint32_t size = (uint32_t)vector.size();
		ar& size;

		if constexpr (std::integral_constant<bool, can_be_processed_as_byte_array<F, T>::value>::value) {
			ar.write(vector.Data, sizeof(T) * vector.Size());
		} else {
			for (const T& e : vector) {
				ar& e;
			}
		}

		return ar;
	}

	template<typename Archive>
	static Archive& load(Archive& ar, ImVector<T>& vector) noexcept {
		uint32_t size = 0;
		ar& size;
		vector.resize(size);

		if constexpr (std::integral_constant<bool, can_be_processed_as_byte_array<F, T>::value>::value) {
			ar.read(vector.Data, sizeof(T) * vector.Size());
		} else {
			for (T& e : vector) {
				ar& e;
			}
		}

		return ar;
	}
};

// 对 ImFontAtlas 的序列化与反序列化来自 https://github.com/ocornut/imgui/issues/6169
template<std::size_t F>
struct serializer<
	type_prop::not_a_fundamental,
	ser_case::use_internal_serializer,
	F,
	ImFontAtlas
> {
	template<typename Archive>
	static Archive& save(Archive& ar, const ImFontAtlas& fontAltas) noexcept {
		ar& fontAltas.TexUvWhitePixel& fontAltas.TexUvLines;

		// 为了方便反序列化，ImFont 两次分别序列化不同部分
		ar& fontAltas.Fonts.size();
		for (ImFont* font : fontAltas.Fonts) {
			ar& font->FontSize;
		}

		ar& fontAltas.TexWidth& fontAltas.TexHeight;
		ar.write(fontAltas.TexPixelsAlpha8, fontAltas.TexWidth * fontAltas.TexHeight);

		for (ImFont* font : fontAltas.Fonts) {
			ar& font->Glyphs;
		}

		return ar;
	}

	template<typename Archive>
	static Archive& load(Archive& ar, ImFontAtlas& fontAltas) noexcept {
		fontAltas.ClearTexData();
		ar& fontAltas.TexUvWhitePixel& fontAltas.TexUvLines;

		int size = 0;
		ar& size;
		for (int i = 0; i < size; ++i) {
			ImFontConfig dummyConfig;
			dummyConfig.FontData = IM_ALLOC(1);
			dummyConfig.FontDataSize = 1;
			dummyConfig.SizePixels = 1.0f;
			ImFont* font = fontAltas.AddFont(&dummyConfig);
			font->ConfigData = &dummyConfig;
			font->ConfigDataCount = 1;
			font->ContainerAtlas = &fontAltas;
			ar& font->FontSize;
		}

		// TexPixelsAlpha8 应在 AddFont 后，AddGlyph前
		ar& fontAltas.TexWidth& fontAltas.TexHeight;
		int totalPixels = fontAltas.TexWidth * fontAltas.TexHeight;
		fontAltas.TexPixelsAlpha8 = (unsigned char*)IM_ALLOC(totalPixels);
		ar.read(fontAltas.TexPixelsAlpha8, totalPixels);

		for (ImFont* font : fontAltas.Fonts) {
			ImVector<ImFontGlyph> glyphs;
			ar& glyphs;

			for (ImFontGlyph& glyph : glyphs) {
				font->AddGlyph(font->ConfigData, glyph.Codepoint, glyph.X0, glyph.Y0, glyph.X1, glyph.Y1,
					glyph.U0, glyph.V0, glyph.U1, glyph.V1, glyph.AdvanceX);
				font->SetGlyphVisible(glyph.Codepoint, glyph.Visible);
			}

			font->BuildLookupTable();
		}

		fontAltas.TexReady = true;

		return ar;
	}
};

}

namespace Magpie::Core {

// 缓存版本
// 当缓存文件结构有更改时更新它，使旧缓存失效
static constexpr const uint32_t FONTS_CACHE_VERSION = 0;

static std::wstring GetCacheFileName(const std::wstring_view& language) noexcept {
	return StrUtils::ConcatW(CommonSharedConstants::CACHE_DIR, L"fonts_", language);
}

void ImGuiFontsCacheManager::Save(std::wstring_view language, const ImFontAtlas& fontAltas) noexcept {
	std::vector<BYTE> buf;
	buf.reserve(131072);

	try {
		yas::vector_ostream os(buf);
		yas::binary_oarchive<yas::vector_ostream<BYTE>, yas::binary> oa(os);

		oa& FONTS_CACHE_VERSION& fontAltas;
	} catch (...) {
		Logger::Get().Error("序列化 ImFontAtlas 失败");
		return;
	}

	if (!Win32Utils::DirExists(CommonSharedConstants::CACHE_DIR)) {
		if (!CreateDirectory(CommonSharedConstants::CACHE_DIR, nullptr)) {
			Logger::Get().Win32Error("创建 cache 文件夹失败");
			return;
		}
	}

	std::wstring cacheFileName = GetCacheFileName(language);
	if (!Win32Utils::WriteFile(cacheFileName.c_str(), buf.data(), buf.size())) {
		Logger::Get().Error("保存字体缓存失败");
	}
}

bool ImGuiFontsCacheManager::Load(std::wstring_view language, ImFontAtlas& fontAltas) noexcept {
	// 不支持在运行时更改语言，因此我们可以缓存字体数据
	static std::vector<BYTE> buf;
	if (buf.empty()) {
		std::wstring cacheFileName = GetCacheFileName(language);
		if (!Win32Utils::FileExists(cacheFileName.c_str())) {
			return false;
		}

		if (!Win32Utils::ReadFile(cacheFileName.c_str(), buf) || buf.empty()) {
			return false;
		}
	}

	try {
		yas::mem_istream mi(buf.data(), buf.size());
		yas::binary_iarchive<yas::mem_istream, yas::binary> ia(mi);

		uint32_t cacheVersion;
		ia& cacheVersion;
		if (cacheVersion != FONTS_CACHE_VERSION) {
			Logger::Get().Info("字体缓存版本不匹配");
			return false;
		}

		ia& fontAltas;
	} catch (...) {
		Logger::Get().Error("反序列化失败");
		return false;
	}

	return true;
}

}
