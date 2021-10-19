#include "pch.h"
#include "EffectCache.h"
#include <yas/mem_streams.hpp>
#include <yas/binary_oarchive.hpp>
#include <yas/binary_iarchive.hpp>
#include <yas/types/std/pair.hpp>
#include <yas/types/std/string.hpp>
#include <yas/types/std/vector.hpp>
#include <filesystem>


template<typename Archive>
void serialize(Archive& ar, ComPtr<ID3DBlob>& o) {
	SIZE_T size = 0;
	ar& size;
	HRESULT hr = D3DCreateBlob(size, o.ReleaseAndGetAddressOf());

	BYTE* buf = (BYTE*)o->GetBufferPointer();
	for (SIZE_T i = 0; i < size; ++i) {
		ar& (*buf++);
	}
}

template<typename Archive>
void serialize(Archive& ar, const ComPtr<ID3DBlob>& o) {
	SIZE_T size = o->GetBufferSize();
	ar& size;

	BYTE* buf = (BYTE*)o->GetBufferPointer();
	for (SIZE_T i = 0; i < size; ++i) {
		ar& (*buf++);
	}
}

template<typename Archive>
void serialize(Archive& ar, const EffectConstantDesc& o) {
	size_t index = o.defaultValue.index();
	ar& index;
	
	if (index == 0) {
		ar& std::get<0>(o.defaultValue);
	} else {
		ar& std::get<1>(o.defaultValue);
	}
	
	ar& o.label;

	index = o.maxValue.index();
	ar& index;
	if (index == 1) {
		ar& std::get<1>(o.maxValue);
	} else if (index == 2) {
		ar& std::get<2>(o.maxValue);
	}

	index = o.minValue.index();
	ar& index;
	if (index == 1) {
		ar& std::get<1>(o.minValue);
	} else if (index == 2) {
		ar& std::get<2>(o.minValue);
	}

	ar& o.name& o.type;
}

template<typename Archive>
void serialize(Archive& ar, EffectConstantDesc& o) {
	size_t index = 0;
	ar& index;

	if (index == 0) {
		o.defaultValue = 0.0f;
		ar& std::get<0>(o.defaultValue);
	} else {
		o.defaultValue = 0;
		ar& std::get<1>(o.defaultValue);
	}

	ar& o.label;

	ar& index;
	if (index == 1) {
		o.maxValue = 0.0f;
		ar& std::get<1>(o.maxValue);
	} else if (index == 2) {
		o.maxValue = 0;
		ar& std::get<2>(o.maxValue);
	}

	ar& index;
	if (index == 1) {
		o.minValue = 0.0f;
		ar& std::get<1>(o.minValue);
	} else if (index == 2) {
		o.minValue = 0;
		ar& std::get<2>(o.minValue);
	}

	ar& o.name& o.type;
}

template<typename Archive>
void serialize(Archive& ar, EffectValueConstantDesc& o) {
	ar& o.name& o.type& o.valueExpr;
}

template<typename Archive>
void serialize(Archive& ar, EffectIntermediateTextureDesc& o) {
	ar& o.format& o.name& o.sizeExpr;
}

template<typename Archive>
void serialize(Archive& ar, EffectSamplerDesc& o) {
	ar& o.filterType& o.name;
}

template<typename Archive>
void serialize(Archive& ar, EffectPassDesc& o) {
	ar& o.inputs& o.outputs& o.cso;
}

template<typename Archive>
void serialize(Archive& ar, EffectDesc& o) {
	ar& o.version& o.outSizeExpr& o.constants& o.valueConstants& o.textures& o.samplers& o.passes;
}


std::wstring GetCacheFileName(const wchar_t* fileName, std::string_view hash) {
	std::wstring file(fileName);

	// 删除文件名中的路径
	std::replace(file.begin(), file.end(), '/', '\\');
	size_t pos = file.find_last_of('\\');
	if (pos != std::wstring::npos) {
		file.erase(0, pos + 1);
	}

	std::replace(file.begin(), file.end(), '.', '_');

	return fmt::format(L"cache/{}_{}.cmfx", file, StrUtils::UTF8ToUTF16(hash));
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
	
	try {
		yas::mem_istream mi(buffer);
		yas::binary_iarchive<yas::mem_istream, yas::binary> ia(mi);
		ia& desc;
	} catch (...) {
		desc = {};
		return false;
	}
	
	return true;
}

void EffectCache::Save(const wchar_t* fileName, std::string_view hash, const EffectDesc& desc) {
	std::vector<BYTE> buf;
	buf.reserve(4096);
	yas::vector_ostream os(buf);

	yas::binary_oarchive<yas::vector_ostream<BYTE>, yas::binary> oa(os);
	oa & desc;

	if (!Utils::DirExists(L"cache\\")) {
		if (!CreateDirectory(L"cache\\", nullptr)) {
			SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("创建 cache 文件夹失败"));
			return;
		}
	}

	std::wstring cacheFileName = GetCacheFileName(fileName, hash);
	Utils::WriteFile(cacheFileName.c_str(), buf.data(), buf.size());
}
