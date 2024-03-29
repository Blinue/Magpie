#include "pch.h"
#include "EffectDrawer.h"
#include "Logger.h"
#include "Win32Utils.h"
#include "MagApp.h"
#include "DeviceResources.h"
#include "TextureLoader.h"
#include "StrUtils.h"
#include "Renderer.h"
#include "CursorManager.h"
#include "GPUTimer.h"
#include "EffectHelper.h"

#pragma push_macro("_UNICODE")
// Conan 的 muparser 不含 UNICODE 支持
#undef _UNICODE
#pragma warning(push)
#pragma warning(disable: 4310)	// 类型强制转换截断常量值
#include <muParser.h>
#pragma warning(push)
#pragma pop_macro("_UNICODE")


namespace Magpie::Core {

bool EffectDrawer::Initialize(
	const EffectDesc& desc,
	const EffectOption& option,
	ID3D11Texture2D* inputTex,
	RECT* outputRect,
	RECT* virtualOutputRect
) {
	_desc = desc;

	SIZE inputSize{};
	{
		D3D11_TEXTURE2D_DESC inputDesc;
		inputTex->GetDesc(&inputDesc);
		inputSize = { (LONG)inputDesc.Width, (LONG)inputDesc.Height };
	}

	const SIZE hostSize = Win32Utils::GetSizeOfRect(MagApp::Get().GetHostWndRect());;
	bool isLastEffect = desc.flags & EffectFlags::LastEffect;
	bool isInlineParams = desc.flags & EffectFlags::InlineParams;

	DeviceResources& dr = MagApp::Get().GetDeviceResources();
	auto d3dDevice = dr.GetD3DDevice();

	static mu::Parser exprParser;
	exprParser.DefineConst("INPUT_WIDTH", inputSize.cx);
	exprParser.DefineConst("INPUT_HEIGHT", inputSize.cy);

	SIZE outputSize{};

	if (desc.outSizeExpr.first.empty()) {
		switch (option.scalingType) {
		case ScalingType::Normal:
		{
			outputSize.cx = std::lroundf(inputSize.cx * option.scale.first);
			outputSize.cy = std::lroundf(inputSize.cy * option.scale.second);
			break;
		}
		case ScalingType::Fit:
		{
			float fillScale = std::min(float(hostSize.cx) / inputSize.cx, float(hostSize.cy) / inputSize.cy);
			outputSize.cx = std::lroundf(inputSize.cx * fillScale * option.scale.first);
			outputSize.cy = std::lroundf(inputSize.cy * fillScale * option.scale.second);
			break;
		}
		case ScalingType::Absolute:
		{
			outputSize.cx = std::lroundf(option.scale.first);
			outputSize.cy = std::lroundf(option.scale.second);
			break;
		}
		case ScalingType::Fill:
		{
			outputSize = hostSize;
			break;
		}
		}
	} else {
		assert(!desc.outSizeExpr.second.empty());

		try {
			exprParser.SetExpr(desc.outSizeExpr.first);
			outputSize.cx = std::lround(exprParser.Eval());

			exprParser.SetExpr(desc.outSizeExpr.second);
			outputSize.cy = std::lround(exprParser.Eval());
		} catch (const mu::ParserError& e) {
			Logger::Get().Error(fmt::format("计算输出尺寸 {} 失败：{}", e.GetExpr(), e.GetMsg()));
			return false;
		}
	}

	if (outputSize.cx <= 0 || outputSize.cy <= 0) {
		Logger::Get().Error("非法的输出尺寸");
		return false;
	}

	exprParser.DefineConst("OUTPUT_WIDTH", outputSize.cx);
	exprParser.DefineConst("OUTPUT_HEIGHT", outputSize.cy);

	_samplers.resize(desc.samplers.size());
	for (UINT i = 0; i < _samplers.size(); ++i) {
		const EffectSamplerDesc& samDesc = desc.samplers[i];
		if (!dr.GetSampler(
			samDesc.filterType == EffectSamplerFilterType::Linear ? D3D11_FILTER_MIN_MAG_MIP_LINEAR : D3D11_FILTER_MIN_MAG_MIP_POINT,
			samDesc.addressType == EffectSamplerAddressType::Clamp ? D3D11_TEXTURE_ADDRESS_CLAMP : D3D11_TEXTURE_ADDRESS_WRAP,
			&_samplers[i])
			) {
			Logger::Get().Error(fmt::format("创建采样器 {} 失败", samDesc.name));
			return false;
		}
	}

	// 创建中间纹理
	// 第一个为 INPUT，最后一个为 OUTPUT
	_textures.resize(desc.textures.size() + 1);
	_textures[0].copy_from(inputTex);
	for (size_t i = 1; i < desc.textures.size(); ++i) {
		const EffectIntermediateTextureDesc& texDesc = desc.textures[i];

		if (!texDesc.source.empty()) {
			// 从文件加载纹理
			size_t delimPos = desc.name.find_last_of('\\');
			std::string texPath = delimPos == std::string::npos 
				? StrUtils::Concat("effects\\", texDesc.source)
				: StrUtils::Concat("effects\\", std::string_view(desc.name.c_str(), delimPos + 1), texDesc.source);
			_textures[i] = TextureLoader::Load(StrUtils::UTF8ToUTF16(texPath).c_str());
			if (!_textures[i]) {
				Logger::Get().Error(fmt::format("加载纹理 {} 失败", texDesc.source));
				return false;
			}

			if (texDesc.format != EffectIntermediateTextureFormat::UNKNOWN) {
				// 检查纹理格式是否匹配
				D3D11_TEXTURE2D_DESC srcDesc{};
				_textures[i]->GetDesc(&srcDesc);
				if (srcDesc.Format != EffectHelper::FORMAT_DESCS[(UINT)texDesc.format].dxgiFormat) {
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
				Logger::Get().Error(fmt::format("计算中间纹理尺寸 {} 失败：{}", e.GetExpr(), e.GetMsg()));
				return false;
			}

			if (texSize.cx <= 0 || texSize.cy <= 0) {
				Logger::Get().Error("非法的中间纹理尺寸");
				return false;
			}

			_textures[i] = dr.CreateTexture2D(
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

	if (!isLastEffect) {
		// 创建输出纹理
		_textures.back() = dr.CreateTexture2D(
			DXGI_FORMAT_R8G8B8A8_UNORM,
			outputSize.cx,
			outputSize.cy,
			D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS
		);

		if (!_textures.back()) {
			Logger::Get().Error("创建纹理失败");
			return false;
		}
	} else {
		_textures.back().copy_from(dr.GetBackBuffer());
	}

	_shaders.resize(desc.passes.size());
	_srvs.resize(desc.passes.size());
	_uavs.resize(desc.passes.size());
	for (UINT i = 0; i < _shaders.size(); ++i) {
		const EffectPassDesc& passDesc = desc.passes[i];

		HRESULT hr = d3dDevice->CreateComputeShader(
			passDesc.cso->GetBufferPointer(), passDesc.cso->GetBufferSize(), nullptr, _shaders[i].put());
		if (FAILED(hr)) {
			Logger::Get().ComError("创建计算着色器失败", hr);
			return false;
		}

		_srvs[i].resize(passDesc.inputs.size());
		for (UINT j = 0; j < passDesc.inputs.size(); ++j) {
			if (!dr.GetShaderResourceView(_textures[passDesc.inputs[j]].get(), &_srvs[i][j])) {
				Logger::Get().Error("GetShaderResourceView 失败");
				return false;
			}
		}

		if (!passDesc.outputs.empty()) {
			_uavs[i].resize(passDesc.outputs.size() * 2);
			for (UINT j = 0; j < passDesc.outputs.size(); ++j) {
				if (!dr.GetUnorderedAccessView(_textures[passDesc.outputs[j]].get(), &_uavs[i][j])) {
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
		} else {
			// 最后一个 pass 输出到 OUTPUT
			_uavs[i].resize(2);
			if (!dr.GetUnorderedAccessView(_textures.back().get(), &_uavs[i][0])) {
				Logger::Get().Error("GetUnorderedAccessView 失败");
				return false;
			}

			D3D11_TEXTURE2D_DESC lastDesc;
			_textures.back()->GetDesc(&lastDesc);

			_dispatches.emplace_back(
				(std::min(lastDesc.Width, (UINT)outputSize.cx) + passDesc.blockSize.first - 1) / passDesc.blockSize.first,
				(std::min(lastDesc.Height, (UINT)outputSize.cy) + passDesc.blockSize.second - 1) / passDesc.blockSize.second
			);
		}
	}

	if (isLastEffect) {
		// 为光标渲染预留空间
		_srvs.back().push_back(nullptr);

		if (!dr.GetSampler(
			MagApp::Get().GetOptions().cursorInterpolationMode == CursorInterpolationMode::NearestNeighbor ? D3D11_FILTER_MIN_MAG_MIP_POINT : D3D11_FILTER_MIN_MAG_MIP_LINEAR,
			D3D11_TEXTURE_ADDRESS_CLAMP,
			&_samplers.emplace_back(nullptr)
			)) {
			Logger::Get().Error("GetSampler 失败");
			return false;
		}
	}

	// 大小必须为 4 的倍数
	size_t builtinConstantCount = isLastEffect ? 16 : 12;
	size_t psStylePassParams = 0;
	for (UINT i = 0, end = (UINT)desc.passes.size() - 1; i < end; ++i) {
		if (desc.passes[i].isPSStyle) {
			psStylePassParams += 4;
		}
	}
	_constants.resize((builtinConstantCount + psStylePassParams + (isInlineParams ? 0 : desc.params.size()) + 3) / 4 * 4);
	// cbuffer __CB2 : register(b1) {
	//     uint2 __inputSize;
	//     uint2 __outputSize;
	//     float2 __inputPt;
	//     float2 __outputPt;
	//     float2 __scale;
	//     int2 __viewport;
	//     [uint4 __offset;]
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

	// 输出尺寸可能比主窗口更大
	RECT virtualOutputRect1{};
	RECT outputRect1{};

	if (isLastEffect) {
		virtualOutputRect1.left = (hostSize.cx - outputSize.cx) / 2;
		virtualOutputRect1.top = (hostSize.cy - outputSize.cy) / 2;
		virtualOutputRect1.right = virtualOutputRect1.left + outputSize.cx;
		virtualOutputRect1.bottom = virtualOutputRect1.top + outputSize.cy;

		outputRect1 = RECT{
			std::max(0L, virtualOutputRect1.left),
			std::max(0L, virtualOutputRect1.top),
			std::min(hostSize.cx, virtualOutputRect1.right),
			std::min(hostSize.cy, virtualOutputRect1.bottom)
		};

		_constants[12].intVal = -std::min(0L, virtualOutputRect1.left);
		_constants[13].intVal = -std::min(0L, virtualOutputRect1.top);
		_constants[10].intVal = outputRect1.right - outputRect1.left + _constants[12].intVal;
		_constants[11].intVal = outputRect1.bottom - outputRect1.top + _constants[13].intVal;
		_constants[14].intVal = outputRect1.left - _constants[12].intVal;
		_constants[15].intVal = outputRect1.top - _constants[13].intVal;
	} else {
		outputRect1 = RECT{ 0, 0, outputSize.cx, outputSize.cy };
		virtualOutputRect1 = outputRect1;

		_constants[10].intVal = outputSize.cx;
		_constants[11].intVal = outputSize.cy;
	}

	if (outputRect) {
		*outputRect = outputRect1;
	}
	if (virtualOutputRect) {
		*virtualOutputRect = virtualOutputRect1;
	}

	// PS 样式的通道需要的参数
	EffectHelper::Constant32* pCurParam = _constants.data() + builtinConstantCount;
	if (psStylePassParams > 0) {
		for (UINT i = 0, end = (UINT)desc.passes.size() - 1; i < end; ++i) {
			if (desc.passes[i].isPSStyle) {
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
			auto it = option.parameters.find(StrUtils::UTF8ToUTF16(paramDesc.name));

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
						Logger::Get().Error(StrUtils::Concat("参数 ", paramDesc.name, " 的值非法"));
						return false;
					}
				}

				pCurParam->intVal = value;
			}

			++pCurParam;
		}
	}

	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = 4 * (UINT)_constants.size();
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	D3D11_SUBRESOURCE_DATA initData{};
	initData.pSysMem = _constants.data();

	HRESULT hr = dr.GetD3DDevice()->CreateBuffer(&bd, &initData, _constantBuffer.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateBuffer 失败", hr);
		return false;
	}

	return true;
}

void EffectDrawer::Draw(UINT& idx, bool noUpdate) {
	auto d3dDC = MagApp::Get().GetDeviceResources().GetD3DDC();
	auto& gpuTimer = MagApp::Get().GetRenderer().GetGPUTimer();

	{
		ID3D11Buffer* t = _constantBuffer.get();
		d3dDC->CSSetConstantBuffers(1, 1, &t);
	}
	d3dDC->CSSetSamplers(0, (UINT)_samplers.size(), _samplers.data());

	for (UINT i = 0; i < _dispatches.size(); ++i) {
		// noUpdate 为真则只渲染最后一个通道
		if (!noUpdate || i == UINT(_dispatches.size() - 1)) {
			_DrawPass(i);
		}

		// 不渲染的通道也在 GPUTimer 中记录
		gpuTimer.OnEndPass(idx++);
	}
}

void EffectDrawer::_DrawPass(UINT i) {
	auto d3dDC = MagApp::Get().GetDeviceResources().GetD3DDC();
	d3dDC->CSSetShader(_shaders[i].get(), nullptr, 0);

	if ((_desc.flags & EffectFlags::LastEffect) && i == _dispatches.size() - 1) {
		// 最后一个效果的最后一个通道负责渲染光标

		// 光标纹理
		CursorManager& cm = MagApp::Get().GetCursorManager();
		if (cm.HasCursor()) {
			ID3D11Texture2D* cursorTex;
			CursorManager::CursorType ct;
			if (cm.GetCursorTexture(&cursorTex, ct)) {
				if (!MagApp::Get().GetDeviceResources().GetShaderResourceView(cursorTex, &_srvs[i].back())) {
					Logger::Get().Error("GetShaderResourceView 出错");
				}
			} else {
				Logger::Get().Error("GetCursorTexture 出错");
			}
		}
	}

	d3dDC->CSSetShaderResources(0, (UINT)_srvs[i].size(), _srvs[i].data());
	UINT uavCount = (UINT)_uavs[i].size() / 2;
	d3dDC->CSSetUnorderedAccessViews(0, uavCount, _uavs[i].data(), nullptr);

	d3dDC->Dispatch(_dispatches[i].first, _dispatches[i].second, 1);

	d3dDC->CSSetUnorderedAccessViews(0, uavCount, _uavs[i].data() + uavCount, nullptr);
}

}
