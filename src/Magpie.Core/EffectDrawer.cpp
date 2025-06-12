#include "pch.h"
#include "EffectDrawer.h"
#include "ScalingOptions.h"
#include "Win32Helper.h"
#include "Logger.h"
#include "DeviceResources.h"
#include "StrHelper.h"
#include "TextureHelper.h"
#include "EffectHelper.h"
#include "DirectXHelper.h"
#include "ScalingWindow.h"
#include "BackendDescriptorStore.h"
#include "EffectsProfiler.h"

namespace Magpie {

EffectDrawer::~EffectDrawer() {
	// [0] 为输入，由前一个 EffectDrawer 管理
	const uint32_t textureCount = (uint32_t)_textures.size();
	for (uint32_t i = 1; i < textureCount; ++i) {
		_descriptorStore->RemoveCache(_textures[i].get());
	}
}

bool EffectDrawer::Initialize(
	const EffectDesc& desc,
	const EffectOption& option,
	DeviceResources& deviceResources,
	BackendDescriptorStore& descriptorStore,
	ID3D11Texture2D** inOutTexture
) noexcept {
	_d3dDC = deviceResources.GetD3DDC();
	_descriptorStore = &descriptorStore;

	SIZE inputSize{};
	{
		D3D11_TEXTURE2D_DESC inputDesc;
		(*inOutTexture)->GetDesc(&inputDesc);
		inputSize = { (LONG)inputDesc.Width, (LONG)inputDesc.Height };
	}

	const SIZE outputSize = _CalcOutputSize(desc, option, inputSize);
	if (outputSize.cx <= 0 || outputSize.cy <= 0) {
		Logger::Get().Error("非法的输出尺寸");
		return false;
	}

	_samplers.resize(desc.samplers.size());
	for (UINT i = 0; i < _samplers.size(); ++i) {
		const EffectSamplerDesc& samDesc = desc.samplers[i];
		_samplers[i] = deviceResources.GetSampler(
			samDesc.filterType == EffectSamplerFilterType::Linear ? D3D11_FILTER_MIN_MAG_MIP_LINEAR : D3D11_FILTER_MIN_MAG_MIP_POINT,
			samDesc.addressType == EffectSamplerAddressType::Clamp ? D3D11_TEXTURE_ADDRESS_CLAMP : D3D11_TEXTURE_ADDRESS_WRAP
		);

		if (!_samplers[i]) {
			Logger::Get().Error(fmt::format("创建采样器 {} 失败", samDesc.name));
			return false;
		}
	}

	// 创建中间纹理
	// 第一个为 INPUT，第二个为 OUTPUT
	_textures.resize(desc.textures.size());
	_textures[0].copy_from(*inOutTexture);

	// 创建输出纹理，格式始终是 DXGI_FORMAT_R8G8B8A8_UNORM
	_textures[1] = DirectXHelper::CreateTexture2D(
		deviceResources.GetD3DDevice(),
		EffectHelper::FORMAT_DESCS[(uint32_t)desc.textures[1].format].dxgiFormat,
		outputSize.cx,
		outputSize.cy,
		D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS
	);

	*inOutTexture = _textures[1].get();
	if (!*inOutTexture) {
		Logger::Get().Error("创建输出纹理失败");
		return false;
	}

	for (size_t i = 2; i < desc.textures.size(); ++i) {
		const EffectIntermediateTextureDesc& texDesc = desc.textures[i];

		if (!texDesc.source.empty()) {
			// 从文件加载纹理
			size_t delimPos = desc.name.find_last_of('\\');
			std::string texPath = delimPos == std::string::npos
				? StrHelper::Concat("effects\\", texDesc.source)
				: StrHelper::Concat("effects\\", std::string_view(desc.name.c_str(), delimPos + 1), texDesc.source);
			_textures[i] = TextureHelper::LoadTexture(
				StrHelper::UTF8ToUTF16(texPath).c_str(), deviceResources.GetD3DDevice());
			if (!_textures[i]) {
				Logger::Get().Error(fmt::format("加载纹理 {} 失败", texDesc.source));
				return false;
			}

			if (texDesc.format != EffectIntermediateTextureFormat::UNKNOWN) {
				// 检查纹理格式是否匹配
				D3D11_TEXTURE2D_DESC srcDesc{};
				_textures[i]->GetDesc(&srcDesc);
				if (srcDesc.Format != EffectHelper::FORMAT_DESCS[(uint32_t)texDesc.format].dxgiFormat) {
					Logger::Get().Error("SOURCE 纹理格式不匹配");
					return false;
				}
			}
		} else {
			SIZE texSize{};
			try {
				_exprParser.SetExpr(texDesc.sizeExpr.first);
				texSize.cx = std::lround(_exprParser.Eval());
				_exprParser.SetExpr(texDesc.sizeExpr.second);
				texSize.cy = std::lround(_exprParser.Eval());
			} catch (const mu::ParserError& e) {
				Logger::Get().Error(fmt::format("计算中间纹理尺寸 {} 失败: {}", e.GetExpr(), e.GetMsg()));
				return false;
			}

			if (texSize.cx <= 0 || texSize.cy <= 0) {
				Logger::Get().Error("非法的中间纹理尺寸");
				return false;
			}

			_textures[i] = DirectXHelper::CreateTexture2D(
				deviceResources.GetD3DDevice(),
				EffectHelper::FORMAT_DESCS[(UINT)texDesc.format].dxgiFormat,
				texSize.cx,
				texSize.cy,
				D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS
			);
			if (!_textures[i]) {
				Logger::Get().Error("创建纹理失败");
				return false;
			}
		}
	}

	uint32_t passCount = (uint32_t)desc.passes.size();
	_shaders.resize(passCount);
	_srvs.resize(passCount);
	_uavs.resize(passCount);
	_dispatches.resize(passCount);

	for (uint32_t i = 0; i < passCount; ++i) {
		const EffectPassDesc& passDesc = desc.passes[i];

		HRESULT hr = deviceResources.GetD3DDevice()->CreateComputeShader(
			passDesc.cso->GetBufferPointer(), passDesc.cso->GetBufferSize(), nullptr, _shaders[i].put());
		if (FAILED(hr)) {
			Logger::Get().ComError("创建计算着色器失败", hr);
			return false;
		}

		_srvs[i].resize(passDesc.inputs.size());
		_uavs[i].resize(passDesc.outputs.size() * 2);
	}

	if (!_UpdatePassResources(desc)) {
		Logger::Get().Error("_UpdatePassResources 失败");
		return false;
	}

	if (!_UpdateConstants(desc, option, deviceResources, inputSize, outputSize)) {
		Logger::Get().Error("_UpdateConstants 失败");
		return false;
	}

	return true;
}

void EffectDrawer::Draw(EffectsProfiler& profiler) const noexcept {
	_PrepareForDraw();

	for (uint32_t i = 0; i < _dispatches.size(); ++i) {
		_DrawPass(i);
		profiler.OnEndPass(_d3dDC);
	}
}

void EffectDrawer::DrawForExport(const EffectDesc& desc, uint32_t passIdx) const noexcept {
	_PrepareForDraw();

	for (uint32_t i : _CalcPassesToDrawForExport(desc, passIdx)) {
		_DrawPass(i);
	}
}

bool EffectDrawer::ResizeTextures(
	const EffectDesc& desc,
	const EffectOption& option,
	DeviceResources& deviceResources,
	ID3D11Texture2D** inOutTexture
) noexcept {
	bool anyChange = false;

	if (*inOutTexture != _textures[0].get()) {
		_textures[0].copy_from(*inOutTexture);
		anyChange = true;
	}

	SIZE inputSize{};
	{
		D3D11_TEXTURE2D_DESC inputDesc;
		_textures[0]->GetDesc(&inputDesc);
		inputSize = { (LONG)inputDesc.Width, (LONG)inputDesc.Height };
	}

	const SIZE outputSize = _CalcOutputSize(desc, option, inputSize);
	if (outputSize.cx <= 0 || outputSize.cy <= 0) {
		Logger::Get().Error("非法的输出尺寸");
		return false;
	}
	
	D3D11_TEXTURE2D_DESC texDesc;
	_textures[1]->GetDesc(&texDesc);

	if ((LONG)texDesc.Width != outputSize.cx || (LONG)texDesc.Height != outputSize.cy) {
		_descriptorStore->RemoveCache(_textures[1].get());

		_textures[1] = DirectXHelper::CreateTexture2D(
			deviceResources.GetD3DDevice(),
			texDesc.Format,
			outputSize.cx,
			outputSize.cy,
			texDesc.BindFlags
		);

		if (!_textures[1]) {
			Logger::Get().Error("创建输出纹理失败");
			return false;
		}

		anyChange = true;
	}

	*inOutTexture = _textures[1].get();

	for (size_t i = 2; i < _textures.size(); ++i) {
		const std::pair<std::string, std::string>& sizeExpr = desc.textures[i].sizeExpr;
		if (sizeExpr.first.empty()) {
			// 从文件加载的纹理无需调整尺寸
			continue;
		}

		SIZE texSize{};
		try {
			_exprParser.SetExpr(sizeExpr.first);
			texSize.cx = std::lround(_exprParser.Eval());
			_exprParser.SetExpr(sizeExpr.second);
			texSize.cy = std::lround(_exprParser.Eval());
		} catch (const mu::ParserError& e) {
			Logger::Get().Error(fmt::format("计算中间纹理尺寸 {} 失败: {}", e.GetExpr(), e.GetMsg()));
			return false;
		}

		if (texSize.cx <= 0 || texSize.cy <= 0) {
			Logger::Get().Error("非法的中间纹理尺寸");
			return false;
		}

		_textures[i]->GetDesc(&texDesc);

		if ((LONG)texDesc.Width != texSize.cx || (LONG)texDesc.Height != texSize.cy) {
			_descriptorStore->RemoveCache(_textures[i].get());

			_textures[i] = DirectXHelper::CreateTexture2D(
				deviceResources.GetD3DDevice(),
				texDesc.Format,
				texSize.cx,
				texSize.cy,
				texDesc.BindFlags
			);

			if (!_textures[i]) {
				Logger::Get().Error("创建纹理失败");
				return false;
			}

			anyChange = true;
		}
	}

	if (!anyChange) {
		return true;
	}

	if (!_UpdatePassResources(desc)) {
		Logger::Get().Error("_UpdatePassResources 失败");
		return false;
	}

	if (!_UpdateConstants(desc, option, deviceResources, inputSize, outputSize)) {
		Logger::Get().Error("_UpdateConstants 失败");
		return false;
	}

	return true;
}

SIZE EffectDrawer::_CalcOutputSize(
	const EffectDesc& desc,
	const EffectOption& option,
	SIZE inputSize
) const noexcept {
	_exprParser.DefineConst("INPUT_WIDTH", inputSize.cx);
	_exprParser.DefineConst("INPUT_HEIGHT", inputSize.cy);

	SIZE outputSize{};
	const std::pair<std::string, std::string>& outputSizeExpr = desc.GetOutputSizeExpr();

	if (outputSizeExpr.first.empty()) {
		const SIZE rendererSize = Win32Helper::GetSizeOfRect(ScalingWindow::Get().RendererRect());

		switch (option.scalingType) {
		case ScalingType::Normal:
		{
			outputSize.cx = std::lroundf(inputSize.cx * option.scale.first);
			outputSize.cy = std::lroundf(inputSize.cy * option.scale.second);
			break;
		}
		case ScalingType::Absolute:
		{
			outputSize.cx = std::lroundf(option.scale.first);
			outputSize.cy = std::lroundf(option.scale.second);
			break;
		}
		case ScalingType::Fit:
		{
			// 窗口模式缩放时将缩放比例为 1 的 Fit 视为 Fill。此时缩放确保是等比例的，但由于舍入
			// 可能存在一个像素的误差。考虑长 100 高 50 的矩形窗口，长调整到 101 时高将四舍五入到
			// 51，再将长调整到 102 高仍是 51，Fit 的计算方式会使这两次调整中有一次存在黑边，而且
			// 也会影响后续计算是否追加 Bicubic。
			const bool treatFitAsFill = ScalingWindow::Get().Options().IsWindowedMode() &&
				IsApprox(option.scale.first, 1.0f) && IsApprox(option.scale.second, 1.0f);

			if (!treatFitAsFill) {
				const float fillScale = std::min(
					float(rendererSize.cx) / inputSize.cx,
					float(rendererSize.cy) / inputSize.cy
				);
				outputSize.cx = std::lroundf(inputSize.cx * fillScale * option.scale.first);
				outputSize.cy = std::lroundf(inputSize.cy * fillScale * option.scale.second);
				break;
			}
			[[fallthrough]];
		}
		case ScalingType::Fill:
		{
			outputSize = rendererSize;
			break;
		}
		default:
			assert(false);
			return {};
		}
	} else {
		assert(!outputSizeExpr.second.empty());

		try {
			_exprParser.SetExpr(outputSizeExpr.first);
			outputSize.cx = std::lround(_exprParser.Eval());

			_exprParser.SetExpr(outputSizeExpr.second);
			outputSize.cy = std::lround(_exprParser.Eval());
		} catch (const mu::ParserError& e) {
			Logger::Get().Error(fmt::format("计算输出尺寸 {} 失败: {}", e.GetExpr(), e.GetMsg()));
			return {};
		}
	}

	_exprParser.DefineConst("OUTPUT_WIDTH", outputSize.cx);
	_exprParser.DefineConst("OUTPUT_HEIGHT", outputSize.cy);

	return outputSize;
}

bool EffectDrawer::_UpdatePassResources(const EffectDesc& desc) noexcept {
	const uint32_t passCount = (uint32_t)desc.passes.size();
	for (uint32_t i = 0; i < passCount; ++i) {
		const SmallVector<uint32_t>& inputs = desc.passes[i].inputs;
		const SmallVector<uint32_t>& outputs = desc.passes[i].outputs;
		const std::pair<uint32_t, uint32_t>& blockSize = desc.passes[i].blockSize;

		for (uint32_t j = 0; j < inputs.size(); ++j) {
			auto srv = _srvs[i][j] = _descriptorStore->GetShaderResourceView(_textures[inputs[j]].get());
			if (!srv) {
				Logger::Get().Error("GetShaderResourceView 失败");
				return false;
			}
		}

		for (uint32_t j = 0; j < outputs.size(); ++j) {
			auto uav = _uavs[i][j] = _descriptorStore->GetUnorderedAccessView(_textures[outputs[j]].get());
			if (!uav) {
				Logger::Get().Error("GetUnorderedAccessView 失败");
				return false;
			}
		}

		D3D11_TEXTURE2D_DESC outputDesc;
		_textures[outputs[0]]->GetDesc(&outputDesc);
		_dispatches[i] = {
			(outputDesc.Width + blockSize.first - 1) / blockSize.first,
			(outputDesc.Height + blockSize.second - 1) / blockSize.second
		};
	}

	return true;
}

bool EffectDrawer::_UpdateConstants(
	const EffectDesc& desc,
	const EffectOption& option,
	DeviceResources& deviceResources,
	SIZE inputSize,
	SIZE outputSize
) noexcept {
	const bool isInlineParams = desc.flags & EffectFlags::InlineParams;

	SmallVector<EffectHelper::Constant32, 32> constants;
	
	// 大小必须为 4 的倍数
	const size_t builtinConstantCount = 10;
	size_t psStylePassParams = 0;
	for (UINT i = 0, end = (UINT)desc.passes.size() - 1; i < end; ++i) {
		if (desc.passes[i].flags & EffectPassFlags::PSStyle) {
			psStylePassParams += 4;
		}
	}
	constants.resize((builtinConstantCount + psStylePassParams + (isInlineParams ? 0 : desc.params.size()) + 3) / 4 * 4);
	// cbuffer __CB1 : register(b0) {
	//     uint2 __inputSize;
	//     uint2 __outputSize;
	//     float2 __inputPt;
	//     float2 __outputPt;
	//     float2 __scale;
	//     [PARAMETERS...]
	// );
	constants[0].uintVal = inputSize.cx;
	constants[1].uintVal = inputSize.cy;
	constants[2].uintVal = outputSize.cx;
	constants[3].uintVal = outputSize.cy;
	constants[4].floatVal = 1.0f / inputSize.cx;
	constants[5].floatVal = 1.0f / inputSize.cy;
	constants[6].floatVal = 1.0f / outputSize.cx;
	constants[7].floatVal = 1.0f / outputSize.cy;
	constants[8].floatVal = outputSize.cx / (FLOAT)inputSize.cx;
	constants[9].floatVal = outputSize.cy / (FLOAT)inputSize.cy;

	// PS 样式的通道需要的参数
	EffectHelper::Constant32* pCurParam = constants.data() + builtinConstantCount;
	if (psStylePassParams > 0) {
		for (UINT i = 0, end = (UINT)desc.passes.size() - 1; i < end; ++i) {
			if (desc.passes[i].flags & EffectPassFlags::PSStyle) {
				D3D11_TEXTURE2D_DESC outputDesc;
				_textures[desc.passes[i].outputs[0]]->GetDesc(&outputDesc);
				pCurParam->uintVal = outputDesc.Width;
				++pCurParam;
				pCurParam->uintVal = outputDesc.Height;
				++pCurParam;
				pCurParam->floatVal = 1.0f / outputDesc.Width;
				++pCurParam;
				pCurParam->floatVal = 1.0f / outputDesc.Height;
				++pCurParam;
			}
		}
	}

	if (!isInlineParams) {
		for (UINT i = 0; i < desc.params.size(); ++i) {
			const auto& paramDesc = desc.params[i];
			auto it = option.parameters.find(paramDesc.name);

			if (paramDesc.constant.index() == 0) {
				const EffectConstant<float>& constant = std::get<0>(paramDesc.constant);
				float value = constant.defaultValue;

				if (it != option.parameters.end()) {
					value = it->second;

					if (value < constant.minValue || value > constant.maxValue) {
						Logger::Get().Error(fmt::format("参数 {} 的值非法", paramDesc.name));
						return false;
					}
				}

				pCurParam->floatVal = value;
			} else {
				const EffectConstant<int>& constant = std::get<1>(paramDesc.constant);
				int value = constant.defaultValue;

				if (it != option.parameters.end()) {
					value = (int)std::lroundf(it->second);

					if ((value < constant.minValue) || (value > constant.maxValue)) {
						Logger::Get().Error(StrHelper::Concat("参数 ", paramDesc.name, " 的值非法"));
						return false;
					}
				}

				pCurParam->intVal = value;
			}

			++pCurParam;
		}
	}

	if (_constantBuffer) {
		// 更新缓冲区
		deviceResources.GetD3DDC()->UpdateSubresource1(
			_constantBuffer.get(), 0, nullptr, constants.data(), 0, 0, D3D11_COPY_DISCARD);
	} else {
		// 创建缓冲区
		D3D11_BUFFER_DESC bd{
			.ByteWidth = 4 * (UINT)constants.size(),
			.Usage = D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11_BIND_CONSTANT_BUFFER
		};

		D3D11_SUBRESOURCE_DATA initData{ .pSysMem = constants.data() };

		HRESULT hr = deviceResources.GetD3DDevice()->CreateBuffer(&bd, &initData, _constantBuffer.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateBuffer 失败", hr);
			return false;
		}
	}

	return true;
}

void EffectDrawer::_DrawPass(uint32_t i) const noexcept {
	_d3dDC->CSSetShader(_shaders[i].get(), nullptr, 0);

	_d3dDC->CSSetShaderResources(0, (UINT)_srvs[i].size(), _srvs[i].data());
	UINT uavCount = (UINT)_uavs[i].size() / 2;
	_d3dDC->CSSetUnorderedAccessViews(0, uavCount, _uavs[i].data(), nullptr);

	_d3dDC->Dispatch(_dispatches[i].first, _dispatches[i].second, 1);

	_d3dDC->CSSetUnorderedAccessViews(0, uavCount, _uavs[i].data() + uavCount, nullptr);
}

static bool IsReadonlyTexture(const EffectDesc& desc, uint32_t texture) noexcept {
	return texture == 0 || !desc.textures[texture].source.empty();
}

// 计算导出某个通道的输出时需要重新渲染的通道
SmallVector<uint32_t> EffectDrawer::_CalcPassesToDrawForExport(
	const EffectDesc& desc,
	uint32_t passIdx
) const noexcept {
	SmallVector<uint32_t> passesToDraw;
	passesToDraw.push_back(passIdx);

	if (passIdx == 0) {
		return passesToDraw;
	}

	const std::vector<EffectPassDesc>& passes = desc.passes;
	const uint32_t end = (uint32_t)passes.size() - 1;

	// 用于记录该通道依赖的输入纹理，格式为 (passIdx, texture)
	SmallVector<std::pair<uint32_t, uint32_t>, 0> depTextures;

	for (uint32_t input : passes[passIdx].inputs) {
		if (!IsReadonlyTexture(desc, input)) {
			depTextures.emplace_back(passIdx, input);
		}
	}

	while (!depTextures.empty()) {
		const auto [curPass, curTexture] = depTextures.pop_back_val();

		// 检查 curTexture 是否会被后面的通道修改
		{
			bool isOverwritten = false;
			for (uint32_t i = curPass + 1; i < end; ++i) {
				const SmallVector<uint32_t>& curOutputs = passes[i].outputs;
				if (std::find(curOutputs.begin(), curOutputs.end(), curTexture) != curOutputs.end()) {
					isOverwritten = true;
					break;
				}
			}
			if (!isOverwritten) {
				continue;
			}
		}

		// 需要重新渲染前一个输出 curTexture 的通道，并带来新的依赖
		for (int i = (int)curPass - 1; i >= 0; --i) {
			const SmallVector<uint32_t>& curOutputs = passes[i].outputs;
			if (std::find(curOutputs.begin(), curOutputs.end(), curTexture) != curOutputs.end()) {
				const uint32_t ui = (uint32_t)i;

				if (std::find(passesToDraw.begin(), passesToDraw.end(), ui) == passesToDraw.end()) {
					passesToDraw.push_back(ui);

					// 作为优化，如果之前的所有通道都需要重新渲染则提前返回
					if ((uint32_t)passesToDraw.size() == passIdx + 1) {
						for (uint32_t j = 0; j <= passIdx; ++j) {
							passesToDraw[j] = j;
						}
						return passesToDraw;
					}

					for (uint32_t input : passes[ui].inputs) {
						if (!IsReadonlyTexture(desc, input)) {
							depTextures.emplace_back(ui, input);
						}
					}
				}

				break;
			}
		}
	}

	std::sort(passesToDraw.begin(), passesToDraw.end());
	return passesToDraw;
}

void EffectDrawer::_PrepareForDraw() const noexcept {
	{
		ID3D11Buffer* t = _constantBuffer.get();
		_d3dDC->CSSetConstantBuffers(0, 1, &t);
	}

	_d3dDC->CSSetSamplers(0, (UINT)_samplers.size(), _samplers.data());
}

}
