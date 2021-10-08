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
	_RemoveComments(source);

	std::string_view sourceView(source);

	// 检查头
	if (!_CheckHeader(sourceView)) {
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

	bool newLine = true;
	BlockType curBlockType = BlockType::Header;
	size_t curBlockOff = 0;

	auto completeCurrentBlock = [&](size_t len, BlockType newBlockType) {
		switch (curBlockType) {
		case BlockType::Header:
			headerBlock = std::string_view(sourceView.data() + curBlockOff, len);
			break;
		case BlockType::Constant:
			constantBlocks.emplace_back(sourceView.data() + curBlockOff, len);
			break;
		case BlockType::Texture:
			textureBlocks.emplace_back(sourceView.data() + curBlockOff, len);
			break;
		case BlockType::Sampler:
			samplerBlocks.emplace_back(sourceView.data() + curBlockOff, len);
			break;
		case BlockType::Common:
			commonBlocks.emplace_back(sourceView.data() + curBlockOff, len);
			break;
		case BlockType::Pass:
			passBlocks.emplace_back(sourceView.data() + curBlockOff, len);
			break;
		default:
			assert(false);
			break;
		}

		curBlockType = newBlockType;
		curBlockOff += len;
	};

	for (size_t i = 0, end = sourceView.size() - 2; i < end; ++i) {
		if (newLine && sourceView[i] == '/' && sourceView[i + 1] == '/' && sourceView[i + 2] == '!') {
			i += 3;

			std::string_view t(sourceView.data() + i, sourceView.size() - 3);
			std::string_view token;
			if (_GetNextToken<false>(t, token)) {
				return 1;
			}
			std::string blockType = Utils::ToUpperCase(token);

			if (blockType == "CONSTANT") {
				completeCurrentBlock(i - 3 - curBlockOff, BlockType::Constant);
			} else if (blockType == "TEXTURE") {
				completeCurrentBlock(i - 3 - curBlockOff, BlockType::Texture);
			} else if (blockType == "SAMPLER") {
				completeCurrentBlock(i - 3 - curBlockOff, BlockType::Sampler);
			} else if (blockType == "COMMON") {
				completeCurrentBlock(i - 3 - curBlockOff, BlockType::Common);
			} else if (blockType == "PASS") {
				completeCurrentBlock(i - 3 - curBlockOff, BlockType::Pass);
			}

			continue;
		} else {
			newLine = false;
		}

		if (sourceView[i] == '\n') {
			newLine = true;
		}
	}

	completeCurrentBlock(sourceView.size() - curBlockOff, BlockType::Header);

	_ResolveHeader(headerBlock, desc);

	for (std::string_view& block : constantBlocks) {
		_ResolveConstants(block, desc);
	}

	return 0;
}

void EffectCompiler::_RemoveComments(std::string& source) {
	std::string result;
	result.reserve(source.size());

	int j = 0;
	for (size_t i = 0; i < source.size(); ++i) {
		if (source[i] == '/') {
			if (source[i + 1] == '/' && source[i + 2] != '!') {
				// 行注释
				i += 2;
				while (source[i] != '\n') {
					++i;
				}

				continue;
			} else if (source[i + 1] == '*') {
				// 块注释
				i += 2;
				while (source[i] != '*' || source[i + 1] != '/') {
					++i;
				}
				++i;

				continue;
			}
		}

		source[j++] = source[i];
	}

	source[j] = '\0';
	source.resize(j);
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

UINT EffectCompiler::_GetNextExpr(std::string_view& source, std::string& expr) {
	size_t pos = source.find('\n');
	if (pos == std::string_view::npos) {
		return 1;
	}

	expr = _RemoveBlanks(source.substr(0, pos));
	if (expr.empty()) {
		return 1;
	}

	source = source.substr(pos);
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

			source = source.substr(i);
			return 0;
		} else if (cur != ' ' && cur != '\t') {
			source = source.substr(i);
			return 1;
		}
	}

	return 1;
}


bool EffectCompiler::_CheckHeader(std::string_view& source) {
	std::string_view token;
	if (_GetNextToken<true>(source, token) || token != "//!") {
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

	size_t off = source.find('\n');
	if (off == std::string_view::npos) {
		return false;
	}

	source = source.substr(off + 1);
	return true;
}

UINT EffectCompiler::_ResolveHeader(std::string_view block, EffectDesc& desc) {
	// 必需的选项：VERSION
	// 可选的选项：WIDTH，HEIGHT

	std::vector<bool> processed(3, false);

	std::string_view token;

	while (true) {
		if (_GetNextToken<true>(block, token) || Utils::ToUpperCase(token) != "//!") {
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
	
	if (!processed[0] || (processed[1] ^ processed[2])) {
		return 1;
	}

	return 0;
}

UINT EffectCompiler::_ResolveConstants(std::string_view block, EffectDesc& desc) {
	// 可选的选项：VALUE，DEFAULT，LABEL，MIN，MAX，INCLUDE_MIN，INCLUDE_MAX

}
