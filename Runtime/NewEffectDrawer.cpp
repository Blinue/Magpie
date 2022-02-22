#include "pch.h"
#include "NewEffectDrawer.h"
#include "Logger.h"
#include "Utils.h"
#include "App.h"
#include "DeviceResources.h"

#pragma push_macro("_UNICODE")
#undef _UNICODE
// Conan 的 muparser 不含 UNICODE 支持
#include <muParser.h>
#pragma pop_macro("_UNICODE")


bool NewEffectDrawer::Initialize(
	const EffectDesc& desc,
	const EffectParams& params,
	ID3D11Texture2D* inputTex,
	ID3D11Texture2D** outputTex,
	RECT* outputRect
) {
	_desc = desc;

	SIZE inputSize{};
	{
		D3D11_TEXTURE2D_DESC inputDesc;
		inputTex->GetDesc(&inputDesc);
		inputSize = { (LONG)inputDesc.Width, (LONG)inputDesc.Height };
	}

	const SIZE hostSize = Utils::GetSizeOfRect(App::Get().GetHostWndRect());;
	bool isLastEffect = desc.Flags & EFFECT_FLAG_LAST_EFFECT;

	DeviceResources& dr = App::Get().GetDeviceResources();
	auto d3dDevice = dr.GetD3DDevice();

	SIZE outputSize{};

	if (desc.outSizeExpr.first.empty()) {
		outputSize = hostSize;

		// scale 属性
		// [+, +]：缩放比例
		// [0, 0]：非等比例缩放到屏幕大小
		// [-, -]：相对于屏幕能容纳的最大等比缩放的比例

		static float DELTA = 1e-5f;

		float scaleX = params.scale.first;
		float scaleY = params.scale.second;

		float fillScale = std::min(float(outputSize.cx) / inputSize.cx, float(outputSize.cy) / inputSize.cy);

		if (scaleX >= DELTA) {
			outputSize.cx = std::lroundf(outputSize.cx * scaleX);
		} else if (scaleX < -DELTA) {
			outputSize.cx = std::lroundf(outputSize.cx * fillScale * -scaleX);
		}

		if (scaleY >= DELTA) {
			outputSize.cy = std::lroundf(outputSize.cy * scaleY);
		} else if (scaleY < -DELTA) {
			outputSize.cy = std::lroundf(outputSize.cy * fillScale * -scaleY);
		}
	} else {
		assert(!desc.outSizeExpr.second.empty());

		try {
			static mu::Parser exprParser;
			exprParser.DefineConst("INPUT_WIDTH", inputSize.cx);
			exprParser.DefineConst("INPUT_HEIGHT", inputSize.cy);

			exprParser.SetExpr(desc.outSizeExpr.first);
			outputSize.cx = std::lround(exprParser.Eval());

			exprParser.SetExpr(desc.outSizeExpr.second);
			outputSize.cy = std::lround(exprParser.Eval());
		} catch (...) {
			Logger::Get().Error("计算输出尺寸失败");
			return false;
		}
	}

	if (outputSize.cx <= 0 || outputSize.cy <= 0) {
		Logger::Get().Error("非法的输出尺寸");
		return false;
	}

	// 大小必须为 4 的倍数
	_constants.resize(((isLastEffect ? 64 : 40) + desc.params.size() + 3) / 4 * 4);
	// cbuffer __CB2 : register(b1) {
	// uint2 __inputSize;
	// uint2 __outputSize;
	// float2 __inputPt;
	// float2 __outputPt;
	// float2 __scale;
	// [uint2 __viewport;]
	// [int4 __offset;]
	// [PARAMETERS]
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

	if (isLastEffect) {
		// 输出尺寸可能比主窗口更大
		RECT virtualOutputRect{};
		virtualOutputRect.left = (hostSize.cx - outputSize.cx) / 2;
		virtualOutputRect.top = (hostSize.cy - hostSize.cy) / 2;
		virtualOutputRect.right = virtualOutputRect.left + outputSize.cx;
		virtualOutputRect.bottom = virtualOutputRect.top + outputSize.cy;

		RECT realOutputRect = {
			std::max(0L, virtualOutputRect.left),
			std::max(0L, virtualOutputRect.top),
			std::min(hostSize.cx, virtualOutputRect.right),
			std::min(hostSize.cy, virtualOutputRect.bottom)
		};

		if (outputRect) {
			*outputRect = realOutputRect;
		}
		/*
		_constants[10].uintVal = outputRect1.right;
		_constants[11].uintVal = outputRect->bottom;
		_constants[12].intVal = outputRect*/
	} else {
		*outputRect = RECT{ 0,0,outputSize.cx, outputSize.cy };
	}
	
	return true;
}

void NewEffectDrawer::Draw()
{
}
