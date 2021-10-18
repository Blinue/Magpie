#include "pch.h"
#include "EffectCache.h"
#include <bitsery/bitsery.h>
#include <bitsery/adapter/buffer.h>
#include <bitsery/traits/vector.h>
#include <bitsery/traits/string.h>
#include <bitsery/ext/std_variant.h>


template <typename S>
void serialize(S& s, EffectDesc& o) {
	
};

std::wstring GetCacheFileName(const wchar_t* fileName, std::string_view hash) {
	std::wstring t(fileName);
	std::replace(t.begin(), t.end(), '.', '_');

	return fmt::format(L"cache/{}_{}.cmfx", t, StrUtils::UTF8ToUTF16(hash));
}


bool EffectCache::Load(const wchar_t* fileName, std::string_view hash, EffectDesc& desc) {
	std::wstring cacheFileName = GetCacheFileName(fileName, hash);

	if (!Utils::FileExists(cacheFileName.c_str())) {
		return false;
	}
	
	std::vector<BYTE> buffer;
	if (!Utils::ReadFile(cacheFileName.c_str(), buffer) || buffer.empty()) {
		return false;
	}

	bitsery::quickDeserialization<bitsery::InputBufferAdapter<std::vector<BYTE>>>({ buffer.begin(), buffer.size() }, desc);

	return false;
}

void EffectCache::Save(const wchar_t* fileName, std::string_view hash, const EffectDesc& desc) {

}
