#include "pch.h"
#include "EffectCompiler.h"
#include <unordered_set>
#include "Utils.h"
#include <bitset>
#include <charconv>
#include "EffectCache.h"
#include "StrUtils.h"


static constexpr const char* META_INDICATOR = "//!";


extern std::shared_ptr<spdlog::logger> logger;


UINT RemoveComments(std::string& source) {
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

				// 保留换行符
				source[j++] = '\n';

				continue;
			} else if (source[i + 1] == '*') {
				// 块注释
				i += 2;

				while (true) {
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

template<bool IncludeNewLine>
static void RemoveLeadingBlanks(std::string_view& source) {
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

template<bool AllowNewLine>
static bool CheckNextToken(std::string_view& source, std::string_view token) {
	RemoveLeadingBlanks<AllowNewLine>(source);

	if (source.size() < token.size() || source.compare(0, token.size(), token) != 0) {
		return false;
	}

	source.remove_prefix(token.size());
	return true;
}

template<bool AllowNewLine>
static UINT GetNextToken(std::string_view& source, std::string_view& value) {
	RemoveLeadingBlanks<AllowNewLine>(source);

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

bool CheckMagic(std::string_view& source) {
	std::string_view token;
	if (!CheckNextToken<true>(source, META_INDICATOR)) {
		return false;
	}

	if (!CheckNextToken<false>(source, "MAGPIE")) {
		return false;
	}
	if (!CheckNextToken<false>(source, "EFFECT")) {
		return false;
	}

	if (GetNextToken<false>(source, token) != 2) {
		return false;
	}

	if (source.empty()) {
		return false;
	}

	return true;
}

UINT GetNextString(std::string_view& source, std::string_view& value) {
	RemoveLeadingBlanks<false>(source);
	size_t pos = source.find('\n');

	value = source.substr(0, pos);
	StrUtils::Trim(value);
	if (value.empty()) {
		return 1;
	}

	source.remove_prefix(std::min(pos + 1, source.size()));
	return 0;
}

template<typename T>
static UINT GetNextNumber(std::string_view& source, T& value) {
	RemoveLeadingBlanks<false>(source);

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

UINT GetNextExpr(std::string_view& source, std::string& expr) {
	RemoveLeadingBlanks<false>(source);
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

UINT ResolveHeader(std::string_view block, EffectDesc& desc) {
	// 必需的选项：VERSION
	// 可选的选项：OUTPUT_WIDTH，OUTPUT_HEIGHT

	std::bitset<3> processed;

	std::string_view token;

	while (true) {
		if (!CheckNextToken<true>(block, META_INDICATOR)) {
			break;
		}

		if (GetNextToken<false>(block, token)) {
			return 1;
		}
		std::string t = StrUtils::ToUpperCase(token);

		if (t == "VERSION") {
			if (processed[0]) {
				return 1;
			}
			processed[0] = true;

			UINT version;
			if (GetNextNumber(block, version)) {
				return 1;
			}

			if (version != EffectCompiler::VERSION) {
				return 1;
			}

			if (GetNextToken<false>(block, token) != 2) {
				return 1;
			}
		} else if (t == "OUTPUT_WIDTH") {
			if (processed[1]) {
				return 1;
			}
			processed[1] = true;

			if (GetNextExpr(block, desc.outSizeExpr.first)) {
				return 1;
			}
		} else if (t == "OUTPUT_HEIGHT") {
			if (processed[2]) {
				return 1;
			}
			processed[2] = true;

			if (GetNextExpr(block, desc.outSizeExpr.second)) {
				return 1;
			}
		} else {
			return 1;
		}
	}

	// HEADER 块不含代码部分
	if (GetNextToken<true>(block, token) != 2) {
		return 1;
	}

	if (!processed[0] || (processed[1] ^ processed[2])) {
		return 1;
	}

	return 0;
}

UINT ResolveConstant(std::string_view block, EffectDesc& desc) {
	// 可选的选项：VALUE，DEFAULT，LABEL，MIN，MAX
	// VALUE 与其他选项互斥
	// 如果无 VALUE 则必须有 DEFAULT

	std::bitset<5> processed;

	std::string_view token;

	if (!CheckNextToken<true>(block, META_INDICATOR)) {
		return 1;
	}

	if (!CheckNextToken<false>(block, "CONSTANT")) {
		return 1;
	}
	if (GetNextToken<false>(block, token) != 2) {
		return 1;
	}

	EffectConstantDesc desc1{};
	EffectValueConstantDesc desc2{};

	std::string_view defaultValue;
	std::string_view minValue;
	std::string_view maxValue;

	while (true) {
		if (!CheckNextToken<true>(block, META_INDICATOR)) {
			break;
		}

		if (GetNextToken<false>(block, token)) {
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

			if (GetNextExpr(block, desc2.valueExpr)) {
				return 1;
			}
		} else if (t == "DEFAULT") {
			if (processed[0] || processed[1]) {
				return 1;
			}
			processed[1] = true;

			if (GetNextString(block, defaultValue)) {
				return 1;
			}
		} else if (t == "LABEL") {
			if (processed[0] || processed[2]) {
				return 1;
			}
			processed[2] = true;

			std::string_view t;
			if (GetNextString(block, t)) {
				return 1;
			}
			desc1.label = t;
		} else if (t == "MIN") {
			if (processed[0] || processed[3]) {
				return 1;
			}
			processed[3] = true;

			if (GetNextString(block, minValue)) {
				return 1;
			}
		} else if (t == "MAX") {
			if (processed[0] || processed[4]) {
				return 1;
			}
			processed[4] = true;

			if (GetNextString(block, maxValue)) {
				return 1;
			}
		} else {
			return 1;
		}
	}

	// 代码部分
	if (GetNextToken<true>(block, token)) {
		return 1;
	}

	if (token == "float") {
		if (processed[0]) {
			desc2.type = EffectConstantType::Float;
		} else {
			desc1.type = EffectConstantType::Float;

			if (!defaultValue.empty()) {
				desc1.defaultValue = 0.0f;
				if (GetNextNumber(defaultValue, std::get<float>(desc1.defaultValue))) {
					return 1;
				}
			}
			if (!minValue.empty()) {
				float value;
				if (GetNextNumber(minValue, value)) {
					return 1;
				}

				if (!defaultValue.empty() && std::get<float>(desc1.defaultValue) < value) {
					return 1;
				}

				desc1.minValue = value;
			}
			if (!maxValue.empty()) {
				float value;
				if (GetNextNumber(maxValue, value)) {
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
				if (GetNextNumber(defaultValue, std::get<int>(desc1.defaultValue))) {
					return 1;
				}
			}
			if (!minValue.empty()) {
				int value;
				if (GetNextNumber(minValue, value)) {
					return 1;
				}

				if (!defaultValue.empty() && std::get<int>(desc1.defaultValue) < value) {
					return 1;
				}

				desc1.minValue = value;
			}
			if (!maxValue.empty()) {
				int value;
				if (GetNextNumber(maxValue, value)) {
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

	if (GetNextToken<true>(block, token)) {
		return 1;
	}
	(processed[0] ? desc2.name : desc1.name) = token;

	if (!CheckNextToken<true>(block, ";")) {
		return 1;
	}

	if (GetNextToken<true>(block, token) != 2) {
		return 1;
	}

	if (processed[0]) {
		desc.valueConstants.emplace_back(std::move(desc2));
	} else {
		desc.constants.emplace_back(std::move(desc1));
	}

	return 0;
}


UINT ResolveTexture(std::string_view block, EffectDesc& desc) {
	// 如果名称为 INPUT 不能有任何选项，含 SOURCE 时不能有任何其他选项
	// 否则必需的选项：FORMAT
	// 可选的选项：WIDTH，HEIGHT

	EffectIntermediateTextureDesc& texDesc = desc.textures.emplace_back();

	std::bitset<4> processed;

	std::string_view token;

	if (!CheckNextToken<true>(block, META_INDICATOR)) {
		return 1;
	}

	if (!CheckNextToken<false>(block, "TEXTURE")) {
		return 1;
	}
	if (GetNextToken<false>(block, token) != 2) {
		return 1;
	}

	while (true) {
		if (!CheckNextToken<true>(block, META_INDICATOR)) {
			break;
		}

		if (GetNextToken<false>(block, token)) {
			return 1;
		}

		std::string t = StrUtils::ToUpperCase(token);

		if (t == "SOURCE") {
			if (processed.any()) {
				return 1;
			}
			processed[0] = true;

			if (GetNextString(block, token)) {
				return 1;
			}

			texDesc.source = token;
		} else if (t == "FORMAT") {
			if (processed[0] || processed[1]) {
				return 1;
			}
			processed[1] = true;

			if (GetNextString(block, token)) {
				return 1;
			}

			static std::unordered_map<std::string, EffectIntermediateTextureFormat> formatMap = {
				{"R8_UNORM", EffectIntermediateTextureFormat::R8_UNORM},
				{"R16_UNORM", EffectIntermediateTextureFormat::R16_UNORM},
				{"R16_FLOAT", EffectIntermediateTextureFormat::R16_FLOAT},
				{"R8G8_UNORM", EffectIntermediateTextureFormat::R8G8_UNORM},
				{"B5G6R5_UNORM", EffectIntermediateTextureFormat::B5G6R5_UNORM},
				{"R16G16_UNORM", EffectIntermediateTextureFormat::R16G16_UNORM},
				{"R16G16_FLOAT", EffectIntermediateTextureFormat::R16G16_FLOAT},
				{"R8G8B8A8_UNORM", EffectIntermediateTextureFormat::R8G8B8A8_UNORM},
				{"B8G8R8A8_UNORM", EffectIntermediateTextureFormat::B8G8R8A8_UNORM},
				{"R10G10B10A2_UNORM", EffectIntermediateTextureFormat::R10G10B10A2_UNORM},
				{"R32_FLOAT", EffectIntermediateTextureFormat::R32_FLOAT},
				{"R11G11B10_FLOAT", EffectIntermediateTextureFormat::R11G11B10_FLOAT},
				{"R32G32_FLOAT", EffectIntermediateTextureFormat::R32G32_FLOAT},
				{"R16G16B16A16_UNORM", EffectIntermediateTextureFormat::R16G16B16A16_UNORM},
				{"R16G16B16A16_FLOAT", EffectIntermediateTextureFormat::R16G16B16A16_FLOAT},
				{"R32G32B32A32_FLOAT", EffectIntermediateTextureFormat::R32G32B32A32_FLOAT}
			};

			auto it = formatMap.find(std::string(token));
			if (it == formatMap.end()) {
				return 1;
			}

			texDesc.format = it->second;
		} else if (t == "WIDTH") {
			if (processed[0] || processed[2]) {
				return 1;
			}
			processed[2] = true;

			if (GetNextExpr(block, texDesc.sizeExpr.first)) {
				return 1;
			}
		} else if (t == "HEIGHT") {
			if (processed[0] || processed[3]) {
				return 1;
			}
			processed[3] = true;

			if (GetNextExpr(block, texDesc.sizeExpr.second)) {
				return 1;
			}
		} else {
			return 1;
		}
	}

	// WIDTH 和 HEIGHT 必须成对出现
	if (processed[2] ^ processed[3]) {
		return 1;
	}

	// 代码部分
	if (!CheckNextToken<true>(block, "Texture2D")) {
		return 1;
	}

	if (GetNextToken<true>(block, token)) {
		return 1;
	}

	if (token == "INPUT") {
		if (processed[1] || processed[2]) {
			return 1;
		}

		// INPUT 已为第一个元素
		desc.textures.pop_back();
	} else {
		// 否则 FORMAT 和 SOURCE 必须二选一
		if (processed[0] == processed[1]) {
			return 1;
		}

		texDesc.name = token;
	}

	if (!CheckNextToken<true>(block, ";")) {
		return 1;
	}

	if (GetNextToken<true>(block, token) != 2) {
		return 1;
	}

	return 0;
}

UINT ResolveSampler(std::string_view block, EffectDesc& desc) {
	// 必选项：FILTER

	EffectSamplerDesc& samDesc = desc.samplers.emplace_back();

	std::string_view token;

	if (!CheckNextToken<true>(block, META_INDICATOR)) {
		return 1;
	}

	if (!CheckNextToken<false>(block, "SAMPLER")) {
		return 1;
	}
	if (GetNextToken<false>(block, token) != 2) {
		return 1;
	}

	if (!CheckNextToken<true>(block, META_INDICATOR)) {
		return 1;
	}

	if (GetNextToken<false>(block, token)) {
		return 1;
	}

	if (StrUtils::ToUpperCase(token) != "FILTER") {
		return 1;
	}

	if (GetNextString(block, token)) {
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
	if (!CheckNextToken<true>(block, "SamplerState")) {
		return 1;
	}

	if (GetNextToken<true>(block, token)) {
		return 1;
	}

	samDesc.name = token;

	if (!CheckNextToken<true>(block, ";")) {
		return 1;
	}

	if (GetNextToken<true>(block, token) != 2) {
		return 1;
	}

	return 0;
}

UINT ResolveCommon(std::string_view& block) {
	// 无选项

	if (!CheckNextToken<true>(block, META_INDICATOR)) {
		return 1;
	}

	if (!CheckNextToken<false>(block, "COMMON")) {
		return 1;
	}

	if (CheckNextToken<true>(block, META_INDICATOR)) {
		return 1;
	}

	if (block.empty()) {
		return 1;
	}

	return 0;
}

class PassInclude : public ID3DInclude {
public:
	HRESULT CALLBACK Open(
		D3D_INCLUDE_TYPE IncludeType,
		LPCSTR pFileName,
		LPCVOID pParentData,
		LPCVOID* ppData,
		UINT* pBytes
	) override {
		std::wstring relativePath = L"effects\\" + StrUtils::UTF8ToUTF16(pFileName);

		std::string file;
		if (!Utils::ReadTextFile(relativePath.c_str(), file)) {
			return E_FAIL;
		}

		char* result = new char[file.size()];
		std::memcpy(result, file.data(), file.size());

		*ppData = result;
		*pBytes = (UINT)file.size();

		return S_OK;
	}

	HRESULT CALLBACK Close(LPCVOID pData) override {
		delete[](char*)pData;
		return S_OK;
	}
};

bool CompilePassPS(std::string_view hlsl, const char* entryPoint, ID3DBlob** blob, UINT passIndex) {
	ComPtr<ID3DBlob> errorMsgs = nullptr;

	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
	PassInclude passInclude;
	HRESULT hr = D3DCompile(hlsl.data(), hlsl.size(), fmt::format("Pass{}", passIndex).c_str(), nullptr, &passInclude,
		entryPoint, "ps_5_0", flags, 0, blob, &errorMsgs);
	if (FAILED(hr)) {
		if (errorMsgs) {
			SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg(
				fmt::format("编译像素着色器失败：{}", (const char*)errorMsgs->GetBufferPointer()), hr));
		}
		return false;
	} else {
		if (errorMsgs) {
			// 显示警告消息
			SPDLOG_LOGGER_WARN(logger,
				"编译像素着色器时产生警告："s + (const char*)errorMsgs->GetBufferPointer());
		}
	}

	return true;
}


struct TPContext {
	ULONG index;
	std::vector<std::string>& passSources;
	std::vector<EffectPassDesc>& passes;
};

void NTAPI TPWork(PTP_CALLBACK_INSTANCE, PVOID Context, PTP_WORK) {
	TPContext* con = (TPContext*)Context;
	ULONG index = InterlockedIncrement(&con->index);

	if (!CompilePassPS(con->passSources[index], "__M", con->passes[index].cso.ReleaseAndGetAddressOf(), index + 1)) {
		con->passes[index].cso = nullptr;
	}
}

UINT ResolvePass(std::string_view block, EffectDesc& desc, std::vector<std::string>& passSources, const std::string& commonHlsl) {
	std::string_view token;

	if (!CheckNextToken<true>(block, META_INDICATOR)) {
		return 1;
	}

	if (!CheckNextToken<false>(block, "PASS")) {
		return 1;
	}

	size_t index;
	if (GetNextNumber(block, index)) {
		return 1;
	}
	if (GetNextToken<false>(block, token) != 2) {
		return 1;
	}

	if (index == 0 || index >= block.size() + 1) {
		return 1;
	}

	if (index > passSources.size() || !passSources[index - 1].empty()) {
		return 1;
	}

	EffectPassDesc& passDesc = desc.passes[index - 1];

	// 用于检查输入和输出中重复的纹理
	std::unordered_map<std::string_view, UINT> texNames;
	for (size_t i = 0; i < desc.textures.size(); ++i) {
		texNames.emplace(desc.textures[i].name, (UINT)i);
	}

	std::bitset<2> processed;

	while (true) {
		if (!CheckNextToken<true>(block, META_INDICATOR)) {
			break;
		}

		if (GetNextToken<false>(block, token)) {
			return 1;
		}

		std::string t = StrUtils::ToUpperCase(token);

		if (t == "BIND") {
			if (processed[0]) {
				return 1;
			}
			processed[0] = true;

			std::string binds;
			if (GetNextExpr(block, binds)) {
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
			if (GetNextExpr(block, saves)) {
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

	std::string& passHlsl = passSources[index - 1];
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

	return 0;
}

UINT ResolvePasses(const std::vector<std::string_view>& blocks, const std::vector<std::string_view>& commons, EffectDesc& desc) {
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

	std::vector<std::string> passSources(blocks.size());
	desc.passes.resize(blocks.size());

	for (size_t i = 0; i < blocks.size(); ++i) {
		if (ResolvePass(blocks[i], desc, passSources, commonHlsl)) {
			SPDLOG_LOGGER_ERROR(logger, fmt::format("解析 Pass{} 失败", i + 1));
			return 1;
		}
	}

	// 确保每个 PASS 都存在
	for (size_t i = 0; i < passSources.size(); ++i) {
		if (passSources[i].empty()) {
			SPDLOG_LOGGER_ERROR(logger, fmt::format("Pass{} 为空", i + 1));
			return 1;
		}
	}

	// 最后一个 PASS 必须输出到 OUTPUT
	if (!desc.passes.back().outputs.empty()) {
		SPDLOG_LOGGER_ERROR(logger, "最后一个 Pass 不能有 SAVE 指令");
		return 1;
	}

	// 编译生成的 hlsl
	assert(!passSources.empty());
	if (passSources.size() == 1) {
		if (!CompilePassPS(passSources[0], "__M", desc.passes[0].cso.ReleaseAndGetAddressOf(), 1)) {
			SPDLOG_LOGGER_ERROR(logger, "编译 Pass1 失败");
			return 1;
		}
	} else {
		// 有多个 Pass，使用线程池加速编译
		TPContext context = {
			0,
			passSources,
			desc.passes
		};

		PTP_WORK work = CreateThreadpoolWork(TPWork, &context, nullptr);

		if (work) {
			for (size_t i = 1; i < passSources.size(); ++i) {
				SubmitThreadpoolWork(work);
			}

			CompilePassPS(passSources[0], "__M", desc.passes[0].cso.ReleaseAndGetAddressOf(), 1);

			WaitForThreadpoolWorkCallbacks(work, FALSE);
			CloseThreadpoolWork(work);

			for (size_t i = 0; i < passSources.size(); ++i) {
				if (!desc.passes[i].cso) {
					SPDLOG_LOGGER_ERROR(logger, fmt::format("编译 Pass{} 失败", i + 1));
					return 1;
				}
			}
		} else {
			SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("CreateThreadpoolWork 失败，回退到单线程编译"));

			// 回退到单线程
			for (size_t i = 0; i < passSources.size(); ++i) {
				if (!CompilePassPS(passSources[i], "__M", desc.passes[i].cso.ReleaseAndGetAddressOf(), (UINT)i + 1)) {
					SPDLOG_LOGGER_ERROR(logger, fmt::format("编译 Pass{} 失败", i + 1));
					return 1;
				}
			}
		}
	}

	return 0;
}


UINT EffectCompiler::Compile(const wchar_t* fileName, EffectDesc& desc) {
	desc = {};

	std::string source;
	if (!Utils::ReadTextFile(fileName, source)) {
		SPDLOG_LOGGER_ERROR(logger, "读取源文件失败");
		return 1;
	}

	if (source.empty()) {
		SPDLOG_LOGGER_ERROR(logger, "源文件为空");
		return 1;
	}

	// 移除注释
	if (RemoveComments(source)) {
		SPDLOG_LOGGER_ERROR(logger, "删除注释失败");
		return 1;
	}

	std::string md5;
	{
		std::vector<BYTE> hash;
		if (!Utils::Hasher::GetInstance().Hash(source.data(), source.size(), hash)) {
			SPDLOG_LOGGER_ERROR(logger, "计算 hash 失败");
		} else {
			md5 = Utils::Bin2Hex(hash.data(), hash.size());

			if (EffectCache::GetInstance().Load(fileName, md5, desc)) {
				// 已从缓存中读取
				return 0;
			}
		}
	}


	std::string_view sourceView(source);

	// 检查头
	if (!CheckMagic(sourceView)) {
		SPDLOG_LOGGER_ERROR(logger, "检查 MagpieFX 头失败");
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

			if (CheckNextToken<true>(t, META_INDICATOR)) {
				std::string_view token;
				if (GetNextToken<false>(t, token)) {
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
		SPDLOG_LOGGER_ERROR(logger, "无 PASS 块");
		return 1;
	}

	if (ResolveHeader(headerBlock, desc)) {
		SPDLOG_LOGGER_ERROR(logger, "解析 Header 块失败");
		return 1;
	}

	for (size_t i = 0; i < constantBlocks.size(); ++i) {
		if (ResolveConstant(constantBlocks[i], desc)) {
			SPDLOG_LOGGER_ERROR(logger, fmt::format("解析 Constant#{} 块失败", i + 1));
			return 1;
		}
	}

	// 纹理第一个元素为 INPUT
	EffectIntermediateTextureDesc& inputTex = desc.textures.emplace_back();
	inputTex.name = "INPUT";
	for (size_t i = 0; i < textureBlocks.size(); ++i) {
		if (ResolveTexture(textureBlocks[i], desc)) {
			SPDLOG_LOGGER_ERROR(logger, fmt::format("解析 Texture#{} 块失败", i + 1));
			return 1;
		}
	}

	for (size_t i = 0; i < samplerBlocks.size(); ++i) {
		if (ResolveSampler(samplerBlocks[i], desc)) {
			SPDLOG_LOGGER_ERROR(logger, fmt::format("解析 Sampler#{} 块失败", i + 1));
			return 1;
		}
	}

	{
		// 确保没有重复的名字
		std::unordered_set<std::string> names;
		for (const auto& d : desc.constants) {
			if (names.find(d.name) != names.end()) {
				SPDLOG_LOGGER_ERROR(logger, "标识符重复");
				return 1;
			}
			names.insert(d.name);
		}
		for (const auto& d : desc.textures) {
			if (names.find(d.name) != names.end()) {
				SPDLOG_LOGGER_ERROR(logger, "标识符重复");
				return 1;
			}
			names.insert(d.name);
		}
		for (const auto& d : desc.samplers) {
			if (names.find(d.name) != names.end()) {
				SPDLOG_LOGGER_ERROR(logger, "标识符重复");
				return 1;
			}
			names.insert(d.name);
		}
	}

	for (size_t i = 0; i < commonBlocks.size(); ++i) {
		if (ResolveCommon(commonBlocks[i])) {
			SPDLOG_LOGGER_ERROR(logger, fmt::format("解析 Common#{} 块失败", i + 1));
			return 1;
		}
	}

	if (ResolvePasses(passBlocks, commonBlocks, desc)) {
		SPDLOG_LOGGER_ERROR(logger, "解析 Pass 块失败");
		return 1;
	}

	EffectCache::GetInstance().Save(fileName, md5, desc);

	return 0;
}
