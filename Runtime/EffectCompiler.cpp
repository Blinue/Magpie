#include "pch.h"
#include "EffectCompiler.h"


UINT EffectCompiler::Compile(const wchar_t* fileName, EffectDesc& desc) {
	desc = {};

	std::string source;
	Utils::ReadTextFile(fileName, source);

	if (source.empty()) {
		return 1;
	}

	// 移除注释
	if (_RemoveComments(source)) {
		return 1;
	}

	std::string_view sourceView(source);

	// 检查头
	if (!_CheckMagic(sourceView)) {
		return 2;
	}

	enum class BlockType {
		Header,
		Constant,
		Texture,
		Sampler,
		Common,
		Pass
	};

	std::string_view headerBlock;
	std::vector<std::string_view> constantBlocks;
	std::vector<std::string_view> textureBlocks;
	std::vector<std::string_view> samplerBlocks;
	std::vector<std::string_view> commonBlocks;
	std::vector<std::string_view> passBlocks;

	BlockType curBlockType = BlockType::Header;
	size_t curBlockOff = 0;

	auto completeCurrentBlock = [&](size_t len, BlockType newBlockType) {
		switch (curBlockType) {
		case BlockType::Header:
			headerBlock = sourceView.substr(curBlockOff, len);
			break;
		case BlockType::Constant:
			constantBlocks.push_back(sourceView.substr(curBlockOff, len));
			break;
		case BlockType::Texture:
			textureBlocks.push_back(sourceView.substr(curBlockOff, len));
			break;
		case BlockType::Sampler:
			samplerBlocks.push_back(sourceView.substr(curBlockOff, len));
			break;
		case BlockType::Common:
			commonBlocks.push_back(sourceView.substr(curBlockOff, len));
			break;
		case BlockType::Pass:
			passBlocks.push_back(sourceView.substr(curBlockOff, len));
			break;
		default:
			assert(false);
			break;
		}

		curBlockType = newBlockType;
		curBlockOff += len;
	};
	
	bool newLine = true;
	std::string_view t = sourceView;
	while (t.size() > 5) {
		if (newLine) {
			// 包含换行符
			size_t len = t.data() - sourceView.data() - curBlockOff + 1;

			if (!_GetNextMetaIndicator(t)) {
				std::string_view token;
				if (_GetNextToken<false>(t, token)) {
					return 1;
				}
				std::string blockType = Utils::ToUpperCase(token);
				
				if (blockType == "CONSTANT") {
					completeCurrentBlock(len, BlockType::Constant);
				} else if (blockType == "TEXTURE") {
					completeCurrentBlock(len, BlockType::Texture);
				} else if (blockType == "SAMPLER") {
					completeCurrentBlock(len, BlockType::Sampler);
				} else if (blockType == "COMMON") {
					completeCurrentBlock(len, BlockType::Common);
				} else if (blockType == "PASS") {
					completeCurrentBlock(len, BlockType::Pass);
				}
			}
		} else {
			t.remove_prefix(1);
		}

		newLine = t[0] == '\n';
	}

	completeCurrentBlock(sourceView.size() - curBlockOff, BlockType::Header);

	if (_ResolveHeader(headerBlock, desc)) {
		return 1;
	}

	for (std::string_view& block : constantBlocks) {
		if (_ResolveConstants(block, desc)) {
			return 1;
		}
	}

	return 0;
}

UINT EffectCompiler::_RemoveComments(std::string& source) {
	// 确保以换行符结尾
	if (source.back() != '\n') {
		source.push_back('\n');
	}

	std::string result;
	result.reserve(source.size());

	int j = 0;
	// 单独处理最后两个字符
	for (size_t i = 0, end = source.size() - 2; i < end; ++i) {
		if (source[i] == '/') {
			if (source[i + 1] == '/' && source[i + 2] != '!') {
				// 行注释
				i += 2;

				// 无需处理越界，因为必定以换行符结尾
				while (source[i] != '\n') {
					++i;
				}

				// 文件结尾
				if (i >= source.size() - 2) {
					source.resize(j);
					return 0;
				}

				continue;
			} else if (source[i + 1] == '*') {
				// 块注释
				i += 2;

				while(true) {
					if (++i >= source.size()) {
						// 未闭合
						return 1;
					}

					if (source[i - 1] == '*' && source[i] == '/') {
						break;
					}
				} 

				// 文件结尾
				if (i >= source.size() - 2) {
					source.resize(j);
					return 0;
				}

				continue;
			}
		}

		source[j++] = source[i];
	}

	// 无需复制最后的换行符
	source[j++] = source[source.size() - 2];
	source.resize(j);
	return 0;
}

void EffectCompiler::_RemoveBlanks(std::string& source) {
	size_t j = 0;
	for (size_t i = 0; i < source.size(); ++i) {
		char c = source[i];
		if (!isspace(c)) {
			source[j++] = c;
		}
	}
	source.resize(j);
}

std::string EffectCompiler::_RemoveBlanks(std::string_view source) {
	std::string result(source.size(), 0);

	size_t j = 0;
	for (size_t i = 0; i < source.size(); ++i) {
		char c = source[i];
		if (!isspace(c)) {
			result[j++] = c;
		}
	}
	result.resize(j);

	return result;
}

UINT EffectCompiler::_GetNextMetaIndicator(std::string_view& source) {
	for (int i = 0, end = source.size() - 2; i < end; ++i) {
		char cur = source[i];

		if (cur == '/') {
			if (source[static_cast<size_t>(i) + 1] == '/' && source[static_cast<size_t>(i) + 2] == '!') {
				source.remove_prefix(static_cast<size_t>(i) + 3);
				return 0;
			} else {
				source.remove_prefix(i);
				return 1;
			}
		} else if (!std::isspace(cur)) {
			source.remove_prefix(i);
			return 1;
		}
	}

	source.remove_prefix(source.size());
	return 1;
}

UINT EffectCompiler::_GetNextExpr(std::string_view& source, std::string& expr) {
	size_t pos = source.find('\n');
	if (pos == std::string_view::npos) {
		return 1;
	}

	expr = _RemoveBlanks(source.substr(0, pos));
	if (expr.empty()) {
		return 1;
	}
	
	source.remove_prefix(pos);
	return 0;
}

UINT EffectCompiler::_GetNextUInt(std::string_view& source, UINT& value) {
	for (int i = 0; i < source.size(); ++i) {
		char cur = source[i];

		if (cur >= '0' && cur <= '9') {
			// 含有数字
			value = cur - '0';

			while (++i < source.size()) {
				cur = source[i];
				if (cur >= '0' && cur <= '9') {
					value = value * 10 + cur - '0';
				} else {
					break;
				}
			}

			source.remove_prefix(i);
			return 0;
		} else if (cur != ' ' && cur != '\t') {
			source.remove_prefix(i);
			return 1;
		}
	}

	source.remove_prefix(source.size());
	return 1;
}


bool EffectCompiler::_CheckMagic(std::string_view& source) {
	std::string_view token;
	if (_GetNextMetaIndicator(source)) {
		return false;
	}
	
	if (_GetNextToken<false>(source, token) || Utils::ToUpperCase(token) != "MAGPIE") {
		return false;
	}
	if (_GetNextToken<false>(source, token) || Utils::ToUpperCase(token) != "EFFECT") {
		return false;
	}

	if (_GetNextToken<false>(source, token) != 2) {
		return false;
	}

	if (source.empty()) {
		return false;
	}

	return true;
}

UINT EffectCompiler::_ResolveHeader(std::string_view block, EffectDesc& desc) {
	// 必需的选项：VERSION
	// 可选的选项：OUTPUT_WIDTH，OUTPUT_HEIGHT

	std::vector<bool> processed(3, false);

	std::string_view token;

	while (true) {
		if (_GetNextMetaIndicator(block)) {
			break;
		}

		if (_GetNextToken<false>(block, token)) {
			return 1;
		}
		std::string t = Utils::ToUpperCase(token);

		if (t == "VERSION") {
			if (processed[0]) {
				return 1;
			}
			processed[0] = true;

			UINT version;
			if (_GetNextUInt(block, version)) {
				return 1;
			}

			if (version != 1) {
				return 1;
			}

			desc.version = version;

			if (_GetNextToken<false>(block, token) != 2) {
				return 1;
			}
		} else if (t == "OUTPUT_WIDTH") {
			if (processed[1]) {
				return 1;
			}
			processed[1] = true;

			if (_GetNextExpr(block, desc.outSizeExpr.first)) {
				return 1;
			}
		} else if (t == "OUTPUT_HEIGHT") {
			if (processed[2]) {
				return 1;
			}
			processed[2] = true;

			if (_GetNextExpr(block, desc.outSizeExpr.second)) {
				return 1;
			}
		} else {
			return 1;
		}
	}

	// HEADER 块不含代码部分
	if (_GetNextToken<true>(block, token) != 2) {
		return 1;
	}
	
	if (!processed[0] || (processed[1] ^ processed[2])) {
		return 1;
	}

	return 0;
}

UINT EffectCompiler::_ResolveConstants(std::string_view block, EffectDesc& desc) {
	// 可选的选项：VALUE，DEFAULT，LABEL，MIN，MAX
	// VALUE 与其他选项互斥

	std::vector<bool> processed(5, false);

	std::string_view token;

	if (_GetNextMetaIndicator(block)) {
		return 1;
	}

	if (_GetNextToken<false>(block, token) || Utils::ToUpperCase(token) != "CONSTANT") {
		return 1;
	}
	if (_GetNextToken<false>(block, token) != 2) {
		return 1;
	}
	
	while (true) {
		if (_GetNextToken<true>(block, token) || token != "//!") {
			break;
		}

		if (_GetNextToken<false>(block, token)) {
			return 1;
		}


		std::string t = Utils::ToUpperCase(token);

		if (t == "VALUE") {

		} else if (t == "DEFAULT") {

		} else if (t == "LABEL") {

		} else if (t == "MIN") {

		} else if (t == "MAX") {

		} else {
			return 1;
		}
	}

	return 0;
}
