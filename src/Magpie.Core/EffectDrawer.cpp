#include "pch.h"
#include "EffectDrawer.h"
#include "ScalingOptions.h"
#include "Win32Helper.h"
#include "Logger.h"
#include "DeviceResources.h"
#include "StrHelper.h"
#include "TextureLoader.h"
#include "EffectHelper.h"
#include "DirectXHelper.h"
#include "ScalingWindow.h"
#include "BackendDescriptorStore.h"
#include "EffectsProfiler.h"

#pragma push_macro("_UNICODE")
// Conan 的 muparser 不含 UNICODE 支持
#undef _UNICODE
#pragma warning(push)
#pragma warning(disable: 4310)	// 类型强制转换截断常量值
#include <muParser.h>
#pragma warning(push)
#pragma pop_macro("_UNICODE")

namespace Magpie {

static SIZE CalcOutputSize(
	const std::pair<std::string, std::string>& outputSizeExpr,
	const EffectOption& option,
	bool treatFitAsFill,
	SIZE swapChainSize,
	SIZE inputSize,
	mu::Parser& exprParser
) noexcept {
	SIZE outputSize{};

	if (outputSizeExpr.first.empty()) {
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
			if (!treatFitAsFill) {
				const float fillScale = std::min(
					float(swapChainSize.cx) / inputSize.cx,
					float(swapChainSize.cy) / inputSize.cy
				);
				outputSize.cx = std::lroundf(inputSize.cx * fillScale * option.scale.first);
				outputSize.cy = std::lroundf(inputSize.cy * fillScale * option.scale.second);
				break;
			}
			[[fallthrough]];
		}
		case ScalingType::Fill:
		{
			outputSize = swapChainSize;
			break;
		}
		default:
			assert(false);
			break;
		}
	} else {
		assert(!outputSizeExpr.second.empty());

		try {
			exprParser.SetExpr(outputSizeExpr.first);
			outputSize.cx = std::lround(exprParser.Eval());

			exprParser.SetExpr(outputSizeExpr.second);
			outputSize.cy = std::lround(exprParser.Eval());
		} catch (const mu::ParserError& e) {
			Logger::Get().Error(fmt::format("计算输出尺寸 {} 失败: {}", e.GetExpr(), e.GetMsg()));
			return {};
		}
	}

	return outputSize;
}

bool EffectDrawer::Initialize(
	const EffectDesc& desc,
	const EffectOption& option,
	bool treatFitAsFill,
	DeviceResources& deviceResources,
	BackendDescriptorStore& descriptorStore,
	ID3D11Texture2D** inOutTexture
) noexcept {
	_d3dDC = deviceResources.GetD3DDC();

	SIZE inputSize{};
	{
		D3D11_TEXTURE2D_DESC inputDesc;
		(*inOutTexture)->GetDesc(&inputDesc);
		inputSize = { (LONG)inputDesc.Width, (LONG)inputDesc.Height };
	}

	static mu::Parser exprParser;
	exprParser.DefineConst("INPUT_WIDTH", inputSize.cx);
	exprParser.DefineConst("INPUT_HEIGHT", inputSize.cy);

	const SIZE swapChainSize = Win32Helper::GetSizeOfRect(ScalingWindow::Get().SwapChainRect());
	const SIZE outputSize = CalcOutputSize(desc.GetOutputSizeExpr(),
		option, treatFitAsFill, swapChainSize, inputSize, exprParser);
	if (outputSize.cx <= 0 || outputSize.cy <= 0) {
		Logger::Get().Error("非法的输出尺寸");
		return false;
	}

	exprParser.DefineConst("OUTPUT_WIDTH", outputSize.cx);
	exprParser.DefineConst("OUTPUT_HEIGHT", outputSize.cy);

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
			_textures[i] = TextureLoader::Load(
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
				exprParser.SetExpr(texDesc.sizeExpr.first);
				texSize.cx = std::lround(exprParser.Eval());
				exprParser.SetExpr(texDesc.sizeExpr.second);
				texSize.cy = std::lround(exprParser.Eval());
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

	_shaders.resize(desc.passes.size());
	_srvs.resize(desc.passes.size());
	_uavs.resize(desc.passes.size());
	for (UINT i = 0; i < _shaders.size(); ++i) {
		const EffectPassDesc& passDesc = desc.passes[i];

		HRESULT hr = deviceResources.GetD3DDevice()->CreateComputeShader(
			passDesc.cso->GetBufferPointer(), passDesc.cso->GetBufferSize(), nullptr, _shaders[i].put());
		if (FAILED(hr)) {
			Logger::Get().ComError("创建计算着色器失败", hr);
			return false;
		}

		_srvs[i].resize(passDesc.inputs.size());
		for (UINT j = 0; j < passDesc.inputs.size(); ++j) {
			auto srv = _srvs[i][j] = descriptorStore.GetShaderResourceView(_textures[passDesc.inputs[j]].get());
			if (!srv) {
				Logger::Get().Error("GetShaderResourceView 失败");
				return false;
			}
		}

		_uavs[i].resize(passDesc.outputs.size() * 2);
		for (UINT j = 0; j < passDesc.outputs.size(); ++j) {
			auto uav = _uavs[i][j] = descriptorStore.GetUnorderedAccessView(_textures[passDesc.outputs[j]].get());
			if (!uav) {
				Logger::Get().Error("GetUnorderedAccessView 失败");
				return false;
			}
		}

		D3D11_TEXTURE2D_DESC outputDesc;
		_textures[passDesc.outputs[0]]->GetDesc(&outputDesc);
		_dispatches.emplace_back(
			(outputDesc.Width + passDesc.blockSize.first - 1) / passDesc.blockSize.first,
			(outputDesc.Height + passDesc.blockSize.second - 1) / passDesc.blockSize.second
		);
	}

	if (!_InitializeConstants(desc, option, deviceResources, inputSize, outputSize)) {
		Logger::Get().Error("_InitializeConstants 失败");
		return false;
	}

	return true;
}

void EffectDrawer::Draw(EffectsProfiler& profiler) const noexcept {
	{
		ID3D11Buffer* t = _constantBuffer.get();
		_d3dDC->CSSetConstantBuffers(0, 1, &t);
	}
	_d3dDC->CSSetSamplers(0, (UINT)_samplers.size(), _samplers.data());

	for (uint32_t i = 0; i < _dispatches.size(); ++i) {
		_DrawPass(i);
		profiler.OnEndPass(_d3dDC);
	}
}

void EffectDrawer::_DrawPass(uint32_t i) const noexcept {
	_d3dDC->CSSetShader(_shaders[i].get(), nullptr, 0);

	_d3dDC->CSSetShaderResources(0, (UINT)_srvs[i].size(), _srvs[i].data());
	UINT uavCount = (UINT)_uavs[i].size() / 2;
	_d3dDC->CSSetUnorderedAccessViews(0, uavCount, _uavs[i].data(), nullptr);

	_d3dDC->Dispatch(_dispatches[i].first, _dispatches[i].second, 1);

	_d3dDC->CSSetUnorderedAccessViews(0, uavCount, _uavs[i].data() + uavCount, nullptr);
}

bool EffectDrawer::_InitializeConstants(
	const EffectDesc& desc,
	const EffectOption& option,
	DeviceResources& deviceResources,
	SIZE inputSize,
	SIZE outputSize
) noexcept {
	const bool isInlineParams = desc.flags & EffectFlags::InlineParams;

	// 大小必须为 4 的倍数
	const size_t builtinConstantCount = 10;
	size_t psStylePassParams = 0;
	for (UINT i = 0, end = (UINT)desc.passes.size() - 1; i < end; ++i) {
		if (desc.passes[i].flags & EffectPassFlags::PSStyle) {
			psStylePassParams += 4;
		}
	}
	_constants.resize((builtinConstantCount + psStylePassParams + (isInlineParams ? 0 : desc.params.size()) + 3) / 4 * 4);
	// cbuffer __CB1 : register(b0) {
	//     uint2 __inputSize;
	//     uint2 __outputSize;
	//     float2 __inputPt;
	//     float2 __outputPt;
	//     float2 __scale;
	//     [PARAMETERS...]
	// );
	_constants[0].uintVal = inputSize.cx;
	_constants[1].uintVal = inputSize.cy;
	_constants[2].uintVal = outputSize.cx;
	_constants[3].uintVal = outputSize.cy;
	_constants[4].floatVal = 1.0f / inputSize.cx;
	_constants[5].floatVal = 1.0f / inputSize.cy;
	_constants[6].floatVal = 1.0f / outputSize.cx;
	_constants[7].floatVal = 1.0f / outputSize.cy;
	_constants[8].floatVal = outputSize.cx / (FLOAT)inputSize.cx;
	_constants[9].floatVal = outputSize.cy / (FLOAT)inputSize.cy;

	// PS 样式的通道需要的参数
	EffectHelper::Constant32* pCurParam = _constants.data() + builtinConstantCount;
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
			auto it = option.parameters.find(StrHelper::UTF8ToUTF16(paramDesc.name));

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

	D3D11_BUFFER_DESC bd{
		.ByteWidth = 4 * (UINT)_constants.size(),
		.Usage = D3D11_USAGE_DEFAULT,
		.BindFlags = D3D11_BIND_CONSTANT_BUFFER
	};

	D3D11_SUBRESOURCE_DATA initData{ .pSysMem = _constants.data() };

	HRESULT hr = deviceResources.GetD3DDevice()->CreateBuffer(&bd, &initData, _constantBuffer.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateBuffer 失败", hr);
		return false;
	}

	return true;
}

}
