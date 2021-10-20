#include "pch.h"
#include "EffectCompiler.h"
#include <unordered_set>
#include "Utils.h"
#include <bitset>
#include "EffectCache.h"


UINT EffectCompiler::Compile(const wchar_t* fileName, EffectDesc& desc) {
	desc = {};

	std::string source;
	if (!Utils::ReadTextFile(fileName, source)) {
		return 1;
	}
	
	if (source.empty()) {
		return 1;
	}
	
	// 移除注释
	if (_RemoveComments(source)) {
		return 1;
	}

	std::string md5;
	{
		std::vector<BYTE> hash;
		if (!Utils::Hasher::GetInstance().Hash(source.data(), source.size(), hash)) {
			SPDLOG_LOGGER_ERROR(logger, "计算 hash 失败");
		} else {
			md5 = Utils::Bin2Hex(hash.data(), hash.size());

			if (EffectCache::Load(fileName, md5, desc)) {
				// 已从缓存中读取
				return 0;
			}
		}
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
				std::string blockType = StrUtils::ToUpperCase(token);
				
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

	// 必须有 PASS 块
	if (passBlocks.empty()) {
		return 1;
	}

	if (_ResolveHeader(headerBlock, desc)) {
		return 1;
	}

	for (const std::string_view& block : constantBlocks) {
		if (_ResolveConstant(block, desc)) {
			return 1;
		}
	}

	// 纹理第一个元素为 INPUT
	EffectIntermediateTextureDesc& inputTex = desc.textures.emplace_back();
	inputTex.name = "INPUT";
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

	{
		// 确保没有重复的名字
		std::unordered_set<std::string> names;
		for (const auto& d : desc.constants) {
			if (names.find(d.name) != names.end()) {
				return 1;
			}
			names.insert(d.name);
		}
		for (const auto& d : desc.textures) {
			if (names.find(d.name) != names.end()) {
				return 1;
			}
			names.insert(d.name);
		}
		for (const auto& d : desc.samplers) {
			if (names.find(d.name) != names.end()) {
				return 1;
			}
			names.insert(d.name);
		}
	}

	for (std::string_view& block : commonBlocks) {
		if (_ResolveCommon(block)) {
			return 1;
		}
	}

	if (_ResolvePasses(passBlocks, commonBlocks, desc)) {
		return 1;
	}

	EffectCache::Save(fileName, md5, desc);

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
	StrUtils::Trim(value);
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

	std::bitset<3> processed;

	std::string_view token;

	while (true) {
		if (!_CheckNextToken<true>(block, _META_INDICATOR)) {
			break;
		}

		if (_GetNextToken<false>(block, token)) {
			return 1;
		}
		std::string t = StrUtils::ToUpperCase(token);

		if (t == "VERSION") {
			if (processed[0]) {
				return 1;
			}
			processed[0] = true;

			UINT version;
			if (_GetNextNumber(block, version)) {
				return 1;
			}

			if (version != VERSION) {
				return 1;
			}

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
	// 如果无 VALUE 则必须有 DEFAULT

	std::bitset<5> processed;

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

		std::string t = StrUtils::ToUpperCase(token);

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

	if (processed[0] == processed[1]) {
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

	std::bitset<3> processed;

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

		std::string t = StrUtils::ToUpperCase(token);

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

		// INPUT 已为第一个元素
		desc.textures.pop_back();
	} else {
		// 否则 FORMAT 为必需
		if (!processed[0]) {
			return 1;
		}

		texDesc.name = token;
	}

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

	if (StrUtils::ToUpperCase(token) != "FILTER") {
		return 1;
	}

	if (_GetNextString(block, token)) {
		return 1;
	}

	std::string filter = StrUtils::ToUpperCase(token);

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

UINT EffectCompiler::_ResolvePasses(const std::vector<std::string_view>& blocks, const std::vector<std::string_view>& commons, EffectDesc& desc) {
	// 可选项：BIND，SAVE

	std::string commonHlsl;

	// 预估需要的空间
	size_t reservedSize = (desc.constants.size() + desc.samplers.size()) * 30;
	for (const auto& c : commons) {
		reservedSize += c.size();
	}
	commonHlsl.reserve(size_t(reservedSize * 1.5f));

	if (!desc.constants.empty() || !desc.valueConstants.empty()) {
		// 常量缓冲区
		commonHlsl.append("cbuffer __C:register(b0){");
		for (const auto& d : desc.constants) {
			commonHlsl.append(d.type == EffectConstantType::Int ? "int " : "float ")
				.append(d.name)
				.append(";");
		}
		for (const auto& d : desc.valueConstants) {
			commonHlsl.append(d.type == EffectConstantType::Int ? "int " : "float ")
				.append(d.name)
				.append(";");
		}
		commonHlsl.append("};");
	}
	if (!desc.samplers.empty()) {
		// 采样器
		for (int i = 0; i < desc.samplers.size(); ++i) {
			commonHlsl.append(fmt::format("SamplerState {}:register(s{});", desc.samplers[i].name, i));
		}
	}
	commonHlsl.push_back('\n');

	for (const auto& c : commons) {
		commonHlsl.append(c);

		if (commonHlsl.back() != '\n') {
			commonHlsl.push_back('\n');
		}
	}
	
	std::string_view token;

	for (std::string_view block : blocks) {
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
		if (passDesc.cso) {
			return 1;
		}

		// 用于检查输入和输出中重复的纹理
		std::unordered_map<std::string_view, UINT> texNames;
		for (size_t i = 0; i < desc.textures.size(); ++i) {
			texNames.emplace(desc.textures[i].name, (UINT)i);
		}

		std::bitset<2> processed;

		while (true) {
			if (!_CheckNextToken<true>(block, _META_INDICATOR)) {
				break;
			}

			if (_GetNextToken<false>(block, token)) {
				return 1;
			}

			std::string t = StrUtils::ToUpperCase(token);

			if (t == "BIND") {
				if (processed[0]) {
					return 1;
				}
				processed[0] = true;

				std::string binds;
				if (_GetNextExpr(block, binds)) {
					return 1;
				}
				
				std::vector<std::string_view> inputs = StrUtils::Split(binds, ',');
				for (const std::string_view& input : inputs) {
					auto it = texNames.find(input);
					if (it == texNames.end()) {
						// 未找到纹理名称
						return 1;
					}

					passDesc.inputs.push_back(it->second);
					texNames.erase(it);
				}
			} else if (t == "SAVE") {
				if (processed[1]) {
					return 1;
				}
				processed[1] = true;

				std::string saves;
				if (_GetNextExpr(block, saves)) {
					return 1;
				}

				std::vector<std::string_view> outputs = StrUtils::Split(saves, ',');
				if (outputs.size() > 8) {
					// 最多 8 个输出
					return 1;
				}
				
				for (const std::string_view& output : outputs) {
					// INPUT 不能作为输出
					if (output == "INPUT") {
						return 1;
					}

					auto it = texNames.find(output);
					if (it == texNames.end()) {
						// 未找到纹理名称
						return 1;
					}

					passDesc.outputs.push_back(it->second);
					texNames.erase(it);
				}
			} else {
				return 1;
			}
		}

		std::string passHlsl;
		passHlsl.reserve(size_t((commonHlsl.size() + block.size() + passDesc.inputs.size() * 30) * 1.5));

		for (int i = 0; i < passDesc.inputs.size(); ++i) {
			passHlsl.append(fmt::format("Texture2D {}:register(t{});", desc.textures[passDesc.inputs[i]].name, i));
		}
		passHlsl.append(commonHlsl).append(block);

		if (passHlsl.back() != '\n') {
			passHlsl.push_back('\n');
		}
		
		// main 函数
		if (passDesc.outputs.size() <= 1) {
			passHlsl.append(fmt::format("float4 __M(float4 p:SV_POSITION,float2 c:TEXCOORD):SV_TARGET"
				"{{return Pass{}(c);}}", index));
		} else {
			// 多渲染目标
			passHlsl.append("void __M(float4 p:SV_POSITION,float2 c:TEXCOORD,out float4 t0:SV_TARGET0,out float4 t1:SV_TARGET1");
			for (int i = 2; i < passDesc.outputs.size(); ++i) {
				passHlsl.append(fmt::format(",out float4 t{0}:SV_TARGET{0}", i));
			}
			passHlsl.append(fmt::format("){{Pass{}(c,t0,t1", index));
			for (int i = 2; i < passDesc.outputs.size(); ++i) {
				passHlsl.append(fmt::format(",t{}", i));
			}
			passHlsl.append(");}");
		}

		if (!Utils::CompilePixelShader(passHlsl, "__M", passDesc.cso.ReleaseAndGetAddressOf())) {
			return 1;
		}
	}

	// 确保每个 PASS 都存在
	for (const auto& p : desc.passes) {
		if (!p.cso) {
			return 1;
		}
	}
	
	// 最后一个 PASS 必须输出到 OUTPUT
	if (!desc.passes.back().outputs.empty()) {
		return 1;
	}

	return 0;
}
