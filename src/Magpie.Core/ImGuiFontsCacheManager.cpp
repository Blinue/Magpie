#include "pch.h"
#include "ImGuiFontsCacheManager.h"
#include <imgui.h>
#include "YasHelper.h"
#include "Logger.h"
#include "ZstdHelper.h"

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

		ar& fontAltas.Fonts.size();
		for (ImFont* font : fontAltas.Fonts) {
			ar& font->Glyphs;
		}

		ar& fontAltas.TexWidth& fontAltas.TexHeight;
		ar.write(fontAltas.TexPixelsAlpha8, fontAltas.TexWidth * fontAltas.TexHeight);

		return ar;
	}

	template<typename Archive>
	static Archive& load(Archive& ar, ImFontAtlas& fontAltas) noexcept {
		/*ar& fontAltas.TexUvWhitePixel& fontAltas.TexUvLines& fontAltas.Fonts;

		uint32_t size = 0;
		ar& size;
		fontAltas.Fonts.resize(size);*/


		return ar;
	}
};

}

namespace Magpie::Core {

static constexpr const int FONTS_CACHE_COMPRESSION_LEVEL = 3;

void ImGuiFontsCacheManager::Save(std::wstring_view language, const ImFontAtlas& fontAltas) noexcept {
	std::vector<BYTE> compressedBuf;
	{
		std::vector<BYTE> buf;
		buf.reserve(4096);

		try {
			yas::vector_ostream os(buf);
			yas::binary_oarchive<yas::vector_ostream<BYTE>, yas::binary> oa(os);

			oa& fontAltas;
		} catch (...) {
			Logger::Get().Error("序列化 ImFontAtlas 失败");
			return;
		}

		if (!ZstdHelper::ZstdCompress(buf, compressedBuf, 3)) {
			Logger::Get().Error("压缩缓存失败");
			return;
		}
	}
}

bool ImGuiFontsCacheManager::Load(std::wstring_view language, ImFontAtlas& fontAltas) noexcept {
	return false;
}

}
