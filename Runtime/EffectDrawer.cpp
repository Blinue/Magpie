#include "pch.h"
#include "EffectDrawer.h"
#include "Logger.h"
#include "Utils.h"
#include "App.h"
#include "DeviceResources.h"
#include "TextureLoader.h"
#include "StrUtils.h"
#include "Renderer.h"
#include "CursorManager.h"
#include <unordered_set>

#pragma push_macro("_UNICODE")
#undef _UNICODE
// Conan 的 muparser 不含 UNICODE 支持
#include <muParser.h>
#pragma pop_macro("_UNICODE")


bool EffectDrawer::Initialize(
	const EffectDesc& desc,
	const EffectParams& params,
	ID3D11Texture2D* inputTex,
	ID3D11Texture2D** outputTex,
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

	const SIZE hostSize = Utils::GetSizeOfRect(App::Get().GetHostWndRect());;
	_isLastEffect = desc.flags & EFFECT_FLAG_LAST_EFFECT;
	bool isInlineParams = desc.flags & EFFECT_FLAG_INLINE_PARAMETERS;

	DeviceResources& dr = App::Get().GetDeviceResources();
	auto d3dDevice = dr.GetD3DDevice();

	static mu::Parser exprParser;
	exprParser.DefineConst("INPUT_WIDTH", inputSize.cx);
	exprParser.DefineConst("INPUT_HEIGHT", inputSize.cy);

	SIZE outputSize{};

	if (desc.outSizeExpr.first.empty()) {
		if (params.scale.has_value()) {
			outputSize = hostSize;

			// scale 属性
			// [+, +]：缩放比例
			// [0, 0]：非等比例缩放到屏幕大小
			// [-, -]：相对于屏幕能容纳的最大等比缩放的比例

			static float DELTA = 1e-5f;

			float scaleX = params.scale.value().first;
			float scaleY = params.scale.value().second;

			float fillScale = std::min(float(outputSize.cx) / inputSize.cx, float(outputSize.cy) / inputSize.cy);

			if (scaleX >= DELTA) {
				outputSize.cx = std::lroundf(inputSize.cx * scaleX);
			} else if (scaleX < -DELTA) {
				outputSize.cx = std::lroundf(inputSize.cx * fillScale * -scaleX);
			}

			if (scaleY >= DELTA) {
				outputSize.cy = std::lroundf(inputSize.cy * scaleY);
			} else if (scaleY < -DELTA) {
				outputSize.cy = std::lroundf(inputSize.cy * fillScale * -scaleY);
			}
		} else {
			outputSize = inputSize;
		}
	} else {
		assert(!desc.outSizeExpr.second.empty());

		if (params.scale.has_value()) {
			Logger::Get().Error("无法指定缩放");
			return false;
		}

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
			_textures[i] = TextureLoader::Load((L"effects\\" + StrUtils::UTF8ToUTF16(texDesc.source)).c_str());
			if (!_textures[i]) {
				Logger::Get().Error(fmt::format("加载纹理 {} 失败", texDesc.source));
				return false;
			}

			if (texDesc.format != EffectIntermediateTextureFormat::UNKNOWN) {
				// 检查纹理格式是否匹配
				D3D11_TEXTURE2D_DESC desc{};
				_textures[i]->GetDesc(&desc);
				if (desc.Format != EffectIntermediateTextureDesc::FORMAT_DESCS[(UINT)texDesc.format].dxgiFormat) {
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
				EffectIntermediateTextureDesc::FORMAT_DESCS[(UINT)texDesc.format].dxgiFormat,
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

	if (!_isLastEffect) {
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

	*outputTex = _textures.back().get();

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

			D3D11_TEXTURE2D_DESC desc;
			_textures[passDesc.outputs[0]]->GetDesc(&desc);
			_dispatches.emplace_back(
				(desc.Width + passDesc.blockSize.first - 1) / passDesc.blockSize.first,
				(desc.Height + passDesc.blockSize.second - 1) / passDesc.blockSize.second
			);
		} else {
			// 最后一个 pass 输出到 OUTPUT
			_uavs[i].resize(2);
			if (!dr.GetUnorderedAccessView(_textures.back().get(), &_uavs[i][0])) {
				Logger::Get().Error("GetUnorderedAccessView 失败");
				return false;
			}

			D3D11_TEXTURE2D_DESC desc;
			_textures.back()->GetDesc(&desc);
			_dispatches.emplace_back(
				(desc.Width + passDesc.blockSize.first - 1) / passDesc.blockSize.first,
				(desc.Height + passDesc.blockSize.second - 1) / passDesc.blockSize.second
			);
		}
	}

	if (_isLastEffect) {
		// 为光标渲染预留空间
		_srvs.back().push_back(nullptr);

		if (!dr.GetSampler(
			App::Get().GetCursorInterpolationMode() == 0 ? D3D11_FILTER_MIN_MAG_MIP_POINT : D3D11_FILTER_MIN_MAG_MIP_LINEAR,
			D3D11_TEXTURE_ADDRESS_CLAMP,
			&_samplers.emplace_back(nullptr)
		)) {
			Logger::Get().Error("GetSampler 失败");
			return false;
		}
	}

	// 大小必须为 4 的倍数
	size_t builtinConstantCount = _isLastEffect ? 16 : 12;
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

	if (_isLastEffect) {
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
	EffectConstant32* pCurParam = _constants.data() + builtinConstantCount;
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
		// 填入参数
		std::unordered_set<std::string_view> paramNames;

		for (UINT i = 0; i < desc.params.size(); ++i) {
			const auto& paramDesc = desc.params[i];
			paramNames.emplace(paramDesc.name);

			auto it = params.params.find(paramDesc.name);

			if (paramDesc.type == EffectConstantType::Float) {
				float value;

				if (it == params.params.end()) {
					value = std::get<float>(paramDesc.defaultValue);
				} else {
					if (it->second.index() == 0) {
						value = std::get<0>(it->second);
					} else {
						value = (float)std::get<1>(it->second);
					}

					if ((paramDesc.minValue.index() == 1 && value < std::get<float>(paramDesc.minValue))
						|| (paramDesc.maxValue.index() == 1 && value > std::get<float>(paramDesc.maxValue))
					) {
						Logger::Get().Error(fmt::format("参数 {} 的值非法", paramDesc.name));
						return false;
					}
				}

				pCurParam->floatVal = value;
			} else {
				int value;

				if (it == params.params.end()) {
					value = std::get<int>(paramDesc.defaultValue);
				} else {
					if (it->second.index() == 0) {
						return false;
					} else {
						value = std::get<int>(it->second);
					}

					if ((paramDesc.minValue.index() == 2 && value < std::get<int>(paramDesc.minValue))
						|| (paramDesc.maxValue.index() == 2 && value > std::get<int>(paramDesc.maxValue))
					) {
						Logger::Get().Error(StrUtils::Concat("参数 ", paramDesc.name," 的值非法"));
						return false;
					}
				}

				pCurParam->intVal = value;
			}

			++pCurParam;
		}

		for (const auto& pair : params.params) {
			if (!paramNames.contains(std::string_view(pair.first))) {
				Logger::Get().Error(StrUtils::Concat("非法参数 ", pair.first));
				return false;
			}
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

void EffectDrawer::Draw() {
	auto d3dDC = App::Get().GetDeviceResources().GetD3DDC();

	{
		ID3D11Buffer* t = _constantBuffer.get();
		d3dDC->CSSetConstantBuffers(1, 1, &t);
	}
	d3dDC->CSSetSamplers(0, (UINT)_samplers.size(), _samplers.data());
	
	for (UINT i = 0; i < _dispatches.size(); ++i) {
		d3dDC->CSSetShader(_shaders[i].get(), nullptr, 0);

		if (_isLastEffect && i == _dispatches.size() - 1) {
			// 最后一个效果的最后一个通道负责渲染光标
			
			// 光标纹理
			CursorManager& cm = App::Get().GetCursorManager();
			if (cm.HasCursor()) {
				ID3D11Texture2D* cursorTex;
				CursorManager::CursorType ct;
				if (cm.GetCursorTexture(&cursorTex, ct)) {
					if (!App::Get().GetDeviceResources().GetShaderResourceView(cursorTex, &_srvs[i].back())) {
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
