#include "pch.h"
#include "EffectCompiler.h"


UINT EffectCompiler::Compile(const wchar_t* fileName, EffectDesc& desc) {
	desc = {};

    std::string source;
    Utils::ReadTextFile(fileName, source);

	if (source.empty()) {
		return 1;
	}

    // 确保以换行结尾，方便解析
    if (source[source.size() - 1] != '\n') {
        source += '\n';
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


	return 0;
}
