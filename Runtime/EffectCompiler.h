#pragma once
#include "pch.h"
#include "EffectDesc.h"
#include "Utils.h"


class EffectCompiler {
public:
	EffectCompiler() = default;

	static UINT Compile(const wchar_t* fileName, EffectDesc& desc);

private:
	static void _RemoveComments(std::string& source);

	static void _RemoveBlanks(std::string& source);

	static std::string _RemoveBlanks(std::string_view source);

	static UINT _GetNextExpr(std::string_view& source, std::string& expr);

	static UINT _GetNextUInt(std::string_view& source, UINT& value);

	template<bool AllowNewLine>
	static UINT _GetNextToken(std::string_view& source, std::string_view& nextToken) {
		for (size_t i = 0; i < source.size(); ++i) {
			char cur = source[i];

			if (std::isalpha(cur) || cur == '_') {
				// 标识符
				for (size_t j = i + 1; j < source.size(); ++j) {
					char c = source[j];

					if (!std::isalnum(c) && c != '_') {
						nextToken = source.substr(i, j - i);
						source = source.substr(j);
						return 0;
					}
				}

				nextToken = source.substr(i);
				source = source.substr(source.size(), 0);
				return 0;
			}

			//! 元数据指示
			if (cur == '/') {
				if (i + 2 < source.size() && source[i + 1] == '/' && source[i + 2] == '!') {
					nextToken = source.substr(i, 3);
					source = source.substr(i + 3);
					return 0;
				} else {
					return 1;
				}
			}

			// 分号
			if (cur == ';') {
				nextToken = source.substr(i, 1);
				source = source.substr(i + 1);
				return 0;
			}

			if constexpr (AllowNewLine) {
				if (!std::isspace(cur)) {
					// 未知字符
					return 1;
				}
			} else {
				if (cur != ' ' && cur != '\t') {
					if (cur == '\n') {
						// 未找到标识符
						return 2;
					} else {
						return 1;
					}
				}
			}
		}

		// 未找到标识符
		return 2;
	}

	static bool _CheckHeader(std::string_view& source);

	static UINT _ResolveHeader(std::string_view block, EffectDesc& desc);

	static UINT _ResolveConstants(std::string_view block, EffectDesc& desc);
};
