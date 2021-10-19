#include "pch.h"
#include "EffectCache.h"
#include <yas/mem_streams.hpp>
#include <yas/binary_oarchive.hpp>
#include <yas/binary_iarchive.hpp>
#include <yas/types/std/pair.hpp>
#include <yas/types/std/string.hpp>
#include <yas/types/std/vector.hpp>
#include "EffectCompiler.h"
#include <regex>


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
	ar& o.outSizeExpr& o.constants& o.valueConstants& o.textures& o.samplers& o.passes;
}


std::wstring ConvertFileName(const wchar_t* fileName) {
	std::wstring file(fileName);

	// 删除文件名中的路径
	size_t pos = file.find_last_of('\\');
	if (pos != std::wstring::npos) {
		file.erase(0, pos + 1);
	}

	std::replace(file.begin(), file.end(), '.', '_');
	return file;
}

std::wstring GetCacheFileName(const wchar_t* fileName, std::string_view hash) {
	return fmt::format(L".\\cache\\{}_{}.cmfx", ConvertFileName(fileName), StrUtils::UTF8ToUTF16(hash));
}


bool EffectCache::Load(const wchar_t* fileName, std::string_view hash, EffectDesc& desc) {
	std::wstring cacheFileName = GetCacheFileName(fileName, hash);

	if (!Utils::FileExists(cacheFileName.c_str())) {
		return false;
	}
	
	std::vector<BYTE> buf;
	if (!Utils::ReadFile(cacheFileName.c_str(), buf) || buf.empty()) {
		return false;
	}

	if (buf.size() < 100) {
		return false;
	}
	
	// 格式：HASH-VERSION-{BODY}

	// 检查哈希
	std::vector<BYTE> bufHash;
	if (!Utils::Hasher::GetInstance()->Hash(
		buf.data() + Utils::Hasher::GetInstance()->GetHashLength(),
		buf.size() - Utils::Hasher::GetInstance()->GetHashLength(),
		bufHash
	)) {
		SPDLOG_LOGGER_ERROR(logger, "计算哈希失败");
		return false;
	}

	if (std::memcmp(buf.data(), bufHash.data(), bufHash.size()) != 0) {
		SPDLOG_LOGGER_INFO(logger, "哈希检查失败");
		return false;
	}

	try {
		yas::mem_istream mi(buf.data() + bufHash.size(), buf.size() - bufHash.size());
		yas::binary_iarchive<yas::mem_istream, yas::binary> ia(mi);

		// 检查版本
		UINT version;
		ia& version;
		if (version != EffectCompiler::VERSION) {
			SPDLOG_LOGGER_INFO(logger, "版本不匹配");
			return false;
		}

		ia& desc;
	} catch (...) {
		SPDLOG_LOGGER_INFO(logger, "反序列化失败");
		desc = {};
		return false;
	}
	
	SPDLOG_LOGGER_INFO(logger, "已读取缓存 " + StrUtils::UTF16ToUTF8(cacheFileName));
	return true;
}

void EffectCache::Save(const wchar_t* fileName, std::string_view hash, const EffectDesc& desc) {
	// 格式：HASH-VERSION-{BODY}

	std::vector<BYTE> buf;
	buf.reserve(4096);
	buf.resize(Utils::Hasher::GetInstance()->GetHashLength());

	try {
		yas::vector_ostream os(buf);
		yas::binary_oarchive<yas::vector_ostream<BYTE>, yas::binary> oa(os);

		oa& EffectCompiler::VERSION;
		oa& desc;
	} catch (...) {
		SPDLOG_LOGGER_ERROR(logger, "序列化失败");
		return;
	}

	// 填充 HASH
	std::vector<BYTE> bufHash;
	if (!Utils::Hasher::GetInstance()->Hash(
		buf.data() + Utils::Hasher::GetInstance()->GetHashLength(),
		buf.size() - Utils::Hasher::GetInstance()->GetHashLength(),
		bufHash
	)) {
		SPDLOG_LOGGER_ERROR(logger, "计算哈希失败");
		return;
	}
	std::memcpy(buf.data(), bufHash.data(), bufHash.size());


	if (!Utils::DirExists(L".\\cache")) {
		if (!CreateDirectory(L".\\cache", nullptr)) {
			SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("创建 cache 文件夹失败"));
			return;
		}
	} else {
		// 删除所有该文件的缓存

		std::wregex regex(fmt::format(L"^{}_[0-9,a-f]{{{}}}.cmfx$", ConvertFileName(fileName),
				Utils::Hasher::GetInstance()->GetHashLength() * 2), std::wregex::optimize | std::wregex::nosubs);

		WIN32_FIND_DATA findData;
		HANDLE hFind = FindFirstFile(L".\\cache\\*", &findData);
		if (hFind != INVALID_HANDLE_VALUE) {
			while (FindNextFile(hFind, &findData)) {
				if (lstrlenW(findData.cFileName) < 8) {
					continue;
				}

				// 正则匹配文件名
				if (!std::regex_match(findData.cFileName, regex)) {
					continue;
				}

				if (!DeleteFile((L".\\cache\\"s + findData.cFileName).c_str())) {
					SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg(fmt::format("删除缓存文件{}失败",
						StrUtils::UTF16ToUTF8(findData.cFileName))));
				}
			}
			FindClose(hFind);
		} else {
			SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("查找缓存文件失败"));
		}
	}
	
	std::wstring cacheFileName = GetCacheFileName(fileName, hash);
	if (!Utils::WriteFile(cacheFileName.c_str(), buf.data(), buf.size())) {
		SPDLOG_LOGGER_ERROR(logger, "保存缓存失败");
	}

	SPDLOG_LOGGER_INFO(logger, "已保存缓存 " + StrUtils::UTF16ToUTF8(cacheFileName));
}
