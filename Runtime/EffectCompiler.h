#pragma once
#include "pch.h"
#include "EffectDesc.h"
#include "StrUtils.h"
#include <charconv>


class EffectCompiler {
public:
	EffectCompiler() = default;

	static UINT Compile(const wchar_t* fileName, EffectDesc& desc);

	static constexpr UINT VERSION = 1;

private:
	class _PassInclude : public ID3DInclude {
	public:
		HRESULT CALLBACK Open(
			D3D_INCLUDE_TYPE IncludeType,
			LPCSTR pFileName,
			LPCVOID pParentData,
			LPCVOID* ppData,
			UINT* pBytes
		) override;

		HRESULT CALLBACK Close(LPCVOID pData) override;
	};

	static bool _CompilePassPS(std::string_view hlsl, const char* entryPoint, ID3DBlob** blob, UINT passIndex);

	static UINT _RemoveComments(std::string& source);

	template<bool IncludeNewLine>
    static void _RemoveLeadingBlanks(std::string_view& source) {
        size_t i = 0;
        for (; i < source.size(); ++i) {
			if constexpr (IncludeNewLine) {
				if (!StrUtils::isspace(source[i])) {
					break;
				}
			} else {
				char c = source[i];
				if (c != ' ' && c != '\t') {
					break;
				}
			}
        }

        source.remove_prefix(i);
    }

	static UINT _GetNextExpr(std::string_view& source, std::string& expr);

	template<typename T>
	static UINT _GetNextNumber(std::string_view& source, T& value) {
		_RemoveLeadingBlanks<false>(source);

		if (source.empty()) {
			return 1;
		}

		const auto& result = std::from_chars(source.data(), source.data() + source.size(), value);
		if ((int)result.ec) {
			return 1;
		}

		// 解析成功
		source.remove_prefix(result.ptr - source.data());
		return 0;
	}

	static UINT _GetNextString(std::string_view& source, std::string_view& value);

	template<bool AllowNewLine>
	static bool _CheckNextToken(std::string_view& source, std::string_view token) {
		_RemoveLeadingBlanks<AllowNewLine>(source);
		
		if (source.size() < token.size() || source.compare(0, token.size(), token) != 0) {
			return false;
		}

		source.remove_prefix(token.size());
		return true;
	}

	template<bool AllowNewLine>
	static UINT _GetNextToken(std::string_view& source, std::string_view& value) {
		_RemoveLeadingBlanks<AllowNewLine>(source);

		if (source.empty()) {
			return 2;
		}

		char cur = source[0];

		if (StrUtils::isalpha(cur) || cur == '_') {
			size_t j = 1;
			for (; j < source.size(); ++j) {
				cur = source[j];

				if (!StrUtils::isalnum(cur) && cur != '_') {
					break;
				}
			}

			value = source.substr(0, j);
			source.remove_prefix(j);
			return 0;
		}

		if constexpr (AllowNewLine) {
			return 1;
		} else {
			return cur == '\n' ? 2 : 1;
		}
	}

	static bool _CheckMagic(std::string_view& source);

	static UINT _ResolveHeader(std::string_view block, EffectDesc& desc);

	static UINT _ResolveConstant(std::string_view block, EffectDesc& desc);

	static UINT _ResolveTexture(std::string_view block, EffectDesc& desc);

	static UINT _ResolveSampler(std::string_view block, EffectDesc& desc);

	static UINT _ResolveCommon(std::string_view& block);

	static void NTAPI _TPWork(PTP_CALLBACK_INSTANCE, PVOID Context, PTP_WORK);

	static UINT _ResolvePasses(const std::vector<std::string_view>& blocks, const std::vector<std::string_view>& commons, EffectDesc& desc);

	static constexpr const char* _META_INDICATOR = "//!";
};
