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

			if (_CheckNextToken<true>(t, _META_INDICATOR)) {
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

			if (t.size() <= 5) {
				break;
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

	for (const std::string_view& block : constantBlocks) {
		if (_ResolveConstant(block, desc)) {
			return 1;
		}
	}

	for (const std::string_view& block : textureBlocks) {
		if (_ResolveTexture(block, desc)) {
			return 1;
		}
	}

	for (const std::string_view& block : samplerBlocks) {
		if (_ResolveSampler(block, desc)) {
			return 1;
		}
	}

	for (std::string_view& block : commonBlocks) {
		if (_ResolveCommon(block)) {
			return 1;
		}
	}

	for (const std::string_view& block : passBlocks) {
		if (_ResolvePass(block, commonBlocks, desc)) {
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

UINT EffectCompiler::_GetNextExpr(std::string_view& source, std::string& expr) {
	_RemoveLeadingBlanks<false>(source);
	size_t size = std::min(source.find('\n') + 1, source.size());

	// 移除空白字符
	expr.resize(size);

	size_t j = 0;
	for (size_t i = 0; i < size; ++i) {
		char c = source[i];
		if (!isspace(c)) {
			expr[j++] = c;
		}
	}
	expr.resize(j);

	if (expr.empty()) {
		return 1;
	}
	
	source.remove_prefix(size);
	return 0;
}

UINT EffectCompiler::_GetNextString(std::string_view& source, std::string_view& value) {
	_RemoveLeadingBlanks<false>(source);
	size_t pos = source.find('\n');

	value = source.substr(0, pos);
	Utils::Trim(value);
	if (value.empty()) {
		return 1;
	}

	source.remove_prefix(std::min(pos + 1, source.size()));
	return 0;
}


bool EffectCompiler::_CheckMagic(std::string_view& source) {
	std::string_view token;
	if (!_CheckNextToken<true>(source, _META_INDICATOR)) {
		return false;
	}
	
	if (!_CheckNextToken<false>(source, "MAGPIE")) {
		return false;
	}
	if (!_CheckNextToken<false>(source, "EFFECT")) {
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
		if (!_CheckNextToken<true>(block, _META_INDICATOR)) {
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
			if (_GetNextNumber(block, version)) {
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

UINT EffectCompiler::_ResolveConstant(std::string_view block, EffectDesc& desc) {
	// 可选的选项：VALUE，DEFAULT，LABEL，MIN，MAX
	// VALUE 与其他选项互斥

	std::vector<bool> processed(5, false);

	std::string_view token;

	if (!_CheckNextToken<true>(block, _META_INDICATOR)) {
		return 1;
	}

	if (!_CheckNextToken<false>(block, "CONSTANT")) {
		return 1;
	}
	if (_GetNextToken<false>(block, token) != 2) {
		return 1;
	}

	EffectConstantDesc desc1{};
	EffectValueConstantDesc desc2{};

	std::string_view defaultValue;
	std::string_view minValue;
	std::string_view maxValue;
	
	while (true) {
		if (!_CheckNextToken<true>(block, _META_INDICATOR)) {
			break;
		}

		if (_GetNextToken<false>(block, token)) {
			return 1;
		}

		std::string t = Utils::ToUpperCase(token);

		if (t == "VALUE") {
			for (int i = 0; i < 5; ++i) {
				if (processed[i]) {
					return 1;
				}
			}
			processed[0] = true;

			if (_GetNextExpr(block, desc2.valueExpr)) {
				return 1;
			}
		} else if (t == "DEFAULT") {
			if (processed[0] || processed[1]) {
				return 1;
			}
			processed[1] = true;
			
			if (_GetNextString(block, defaultValue)) {
				return 1;
			}
		} else if (t == "LABEL") {
			if (processed[0] || processed[2]) {
				return 1;
			}
			processed[2] = true;

			std::string_view t;
			if (_GetNextString(block, t)) {
				return 1;
			}
			desc1.label = t;
		} else if (t == "MIN") {
			if (processed[0] || processed[3]) {
				return 1;
			}
			processed[3] = true;

			if (_GetNextString(block, minValue)) {
				return 1;
			}
		} else if (t == "MAX") {
			if (processed[0] || processed[4]) {
				return 1;
			}
			processed[4] = true;

			if (_GetNextString(block, maxValue)) {
				return 1;
			}
		} else {
			return 1;
		}
	}

	// 代码部分
	if (_GetNextToken<true>(block, token)) {
		return 1;
	}

	if (token == "float") {
		if (processed[0]) {
			desc2.type = EffectConstantType::Float;
		} else {
			desc1.type = EffectConstantType::Float;

			if (!defaultValue.empty()) {
				desc1.defaultValue = 0.0f;
				if (_GetNextNumber(defaultValue, std::get<float>(desc1.defaultValue))) {
					return 1;
				}
			}
			if (!minValue.empty()) {
				float value;
				if (_GetNextNumber(minValue, value)) {
					return 1;
				}

				if (!defaultValue.empty() && std::get<float>(desc1.defaultValue) < value) {
					return 1;
				}

				desc1.minValue = value;
			}
			if (!maxValue.empty()) {
				float value;
				if (_GetNextNumber(maxValue, value)) {
					return 1;
				}

				if (!defaultValue.empty() && std::get<float>(desc1.defaultValue) > value) {
					return 1;
				}

				if (!minValue.empty() && std::get<float>(desc1.minValue) > value) {
					return 1;
				}

				desc1.maxValue = value;
			}
		}
	} else if (token == "int") {
		if (processed[0]) {
			desc2.type = EffectConstantType::Int;
		} else {
			desc1.type = EffectConstantType::Int;

			if (!defaultValue.empty()) {
				desc1.defaultValue = 0;
				if (_GetNextNumber(defaultValue, std::get<int>(desc1.defaultValue))) {
					return 1;
				}
			}
			if (!minValue.empty()) {
				int value;
				if (_GetNextNumber(minValue, value)) {
					return 1;
				}

				if (!defaultValue.empty() && std::get<int>(desc1.defaultValue) < value) {
					return 1;
				}

				desc1.minValue = value;
			}
			if (!maxValue.empty()) {
				int value;
				if (_GetNextNumber(maxValue, value)) {
					return 1;
				}

				if (!defaultValue.empty() && std::get<int>(desc1.defaultValue) > value) {
					return 1;
				}

				if (!minValue.empty() && std::get<int>(desc1.minValue) > value) {
					return 1;
				}

				desc1.maxValue = value;
			}
		}
	} else {
		return 1;
	}

	if (_GetNextToken<true>(block, token)) {
		return 1;
	}
	(processed[0] ? desc2.name : desc1.name) = token;

	if (!_CheckNextToken<true>(block, ";")) {
		return 1;
	}

	if (_GetNextToken<true>(block, token) != 2) {
		return 1;
	}

	if (processed[0]) {
		desc.valueConstants.emplace_back(std::move(desc2));
	} else {
		desc.constants.emplace_back(std::move(desc1));
	}

	return 0;
}

UINT EffectCompiler::_ResolveTexture(std::string_view block, EffectDesc& desc) {
	// 如果名称为 INPUT 不能有任何选项
	// 否则必需的选项：FORMAT
	// 可选的选项：WIDTH，HEIGHT

	EffectIntermediateTextureDesc& texDesc = desc.textures.emplace_back();

	std::vector<bool> processed(3, false);

	std::string_view token;

	if (!_CheckNextToken<true>(block, _META_INDICATOR)) {
		return 1;
	}

	if (!_CheckNextToken<false>(block, "TEXTURE")) {
		return 1;
	}
	if (_GetNextToken<false>(block, token) != 2) {
		return 1;
	}

	while (true) {
		if (!_CheckNextToken<true>(block, _META_INDICATOR)) {
			break;
		}

		if (_GetNextToken<false>(block, token)) {
			return 1;
		}

		std::string t = Utils::ToUpperCase(token);

		if (t == "FORMAT") {
			if (processed[0]) {
				return 1;
			}
			processed[0] = true;

			if (_GetNextString(block, token)) {
				return 1;
			}

			if (token == "B8G8R8A8_UNORM") {
				texDesc.format = EffectIntermediateTextureFormat::B8G8R8A8_UNORM;
			} else if (token == "R16G16B16A16_FLOAT") {
				texDesc.format = EffectIntermediateTextureFormat::R16G16B16A16_FLOAT;
			} else {
				return 1;
			}
		} else if (t == "WIDTH") {
			if (processed[1]) {
				return 1;
			}
			processed[1] = true;

			if (_GetNextExpr(block, texDesc.sizeExpr.first)) {
				return 1;
			}
		} else if (t == "HEIGHT") {
			if (processed[2]) {
				return 1;
			}
			processed[2] = true;

			if (_GetNextExpr(block, texDesc.sizeExpr.second)) {
				return 1;
			}
		} else  {
			return 1;
		}
	}

	// WIDTH 和 HEIGHT 必须成对出现
	if (processed[1] ^ processed[2]) {
		return 1;
	}

	// 代码部分
	if (!_CheckNextToken<true>(block, "Texture2D")) {
		return 1;
	}
	
	if (_GetNextToken<true>(block, token)) {
		return 1;
	}

	if (token == "INPUT") {
		if (processed[0] || processed[1]) {
			return 1;
		}
	} else {
		// 否则 FORMAT 为必需
		if (!processed[0]) {
			return 1;
		}
	}

	texDesc.name = token;

	if (!_CheckNextToken<true>(block, ";")) {
		return 1;
	}

	if (_GetNextToken<true>(block, token) != 2) {
		return 1;
	}

	return 0;
}

UINT EffectCompiler::_ResolveSampler(std::string_view block, EffectDesc& desc) {
	// 必选项：FILTER
	
	EffectSamplerDesc& samDesc = desc.samplers.emplace_back();

	std::string_view token;

	if (!_CheckNextToken<true>(block, _META_INDICATOR)) {
		return 1;
	}

	if (!_CheckNextToken<false>(block, "SAMPLER")) {
		return 1;
	}
	if (_GetNextToken<false>(block, token) != 2) {
		return 1;
	}

	if (!_CheckNextToken<true>(block, _META_INDICATOR)) {
		return 1;
	}

	if (_GetNextToken<false>(block, token)) {
		return 1;
	}

	if (Utils::ToUpperCase(token) != "FILTER") {
		return 1;
	}

	if (_GetNextString(block, token)) {
		return 1;
	}

	std::string filter = Utils::ToUpperCase(token);

	if (filter == "LINEAR") {
		samDesc.filterType = EffectSamplerFilterType::Linear;
	} else if (filter == "POINT") {
		samDesc.filterType = EffectSamplerFilterType::Point;
	} else {
		return 1;
	}

	// 代码部分
	if (!_CheckNextToken<true>(block, "SamplerState")) {
		return 1;
	}

	if (_GetNextToken<true>(block, token)) {
		return 1;
	}

	samDesc.name = token;

	if (!_CheckNextToken<true>(block, ";")) {
		return 1;
	}

	if (_GetNextToken<true>(block, token) != 2) {
		return 1;
	}

	return 0;
}

UINT EffectCompiler::_ResolveCommon(std::string_view& block) {
	// 无选项

	if (!_CheckNextToken<true>(block, _META_INDICATOR)) {
		return 1;
	}

	if (!_CheckNextToken<false>(block, "COMMON")) {
		return 1;
	}

	if (_CheckNextToken<true>(block, _META_INDICATOR)) {
		return 1;
	}

	if (block.empty()) {
		return 1;
	}

	return 0;
}

UINT EffectCompiler::_ResolvePass(std::string_view block, const std::vector<std::string_view>& commons, EffectDesc& desc) {
	// 可选项：BIND，SAVE

	std::string_view token;

	if (!_CheckNextToken<true>(block, _META_INDICATOR)) {
		return 1;
	}

	if (!_CheckNextToken<false>(block, "PASS")) {
		return 1;
	}

	UINT index;
	if (_GetNextNumber(block, index)) {
		return 1;
	}
	if (_GetNextToken<false>(block, token) != 2) {
		return 1;
	}

	if (desc.passes.size() < index) {
		desc.passes.resize(index);
	}

	EffectPassDesc& passDesc = desc.passes[static_cast<size_t>(index) - 1];
	if (!passDesc.cso.empty()) {
		return 1;
	}



	return 0;
}
